/* Global, SSA-based optimizations using comparison identities.
   Copyright (C) 2024 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 3, or (at your option) any
later version.

GCC is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

/* (x - y) CMP 0 is equivalent to x CMP y where x and y are signed integers and
   CMP is <, <=, >, or >=.  Similarly, 0 CMP (x - y) is equivalent to y CMP x.
   As reported in PR middle-end/113680, this equivalence does not hold for
   types other than signed integers.  When it comes to conditions, the former
   was translated to a combination of sub and test, whereas the latter was
   translated to a single cmp.  Thus, this optimization pass tries to optimize
   the former to the latter.

   When `-fwrapv` is enabled, GCC treats the overflow of signed integers as
   defined behavior, specifically, wrapping around according to two's
   complement arithmetic.  This has implications for optimizations that
   rely on the standard behavior of signed integers, where overflow is
   undefined.  Consider the example given:

	long long llmax = __LONG_LONG_MAX__;
	long long llmin = -llmax - 1;

   Here, `llmax - llmin` effectively becomes `llmax - (-llmax - 1)`, which
   simplifies to `2 * llmax + 1`.  Given that `llmax` is the maximum value for
   a `long long`, this calculation overflows in a defined manner
   (wrapping around), which under `-fwrapv` is a legal operation that produces
   a negative value due to two's complement wraparound.  Therefore,
   `llmax - llmin < 0` is true.

   However, the direct comparison `llmax < llmin` is false since `llmax` is the
   maximum possible value and `llmin` is the minimum.  Hence, optimizations
   that rely on the equivalence of `(x - y) CMP 0` to `x CMP y` (and vice versa)
   cannot be safely applied when `-fwrapv` is enabled.  This is why this
   optimization pass is disabled under `-fwrapv`.

   This optimization pass must run before the Jump Threading pass and
   the VRP pass, as it may modify conditions. For example, in the VRP pass:

	(1)
	  int diff = x - y;
	  if (diff > 0)
	    foo();
	  if (diff < 0)
	    bar();

   The second condition would be converted to diff != 0 in the VRP pass because
   we know the postcondition of the first condition is diff <= 0, and then
   diff != 0 is cheaper than diff < 0. If we apply this pass after this VRP,
   we get:

	(2)
	  int diff = x - y;
	  if (x > y)
	    foo();
	  if (diff != 0)
	    bar();

   This generates sub and test for the second condition and cmp for the first
   condition. However, if we apply this pass beforehand, we simply get:

	(3)
	  int diff = x - y;
	  if (x > y)
	    foo();
	  if (x < y)
	    bar();

   In this code, diff will be eliminated as a dead code, and sub and test will
   not be generated, which is more efficient.

   For the Jump Threading pass, without this optimization pass, (1) and (3)
   above are recognized as different, which prevents TCO.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "backend.h"
#include "tree.h"
#include "gimple.h"
#include "tree-pass.h"
#include "ssa.h"
#include "gimple-iterator.h"
#include "domwalk.h"

/* True if VAR is a signed integer, false otherwise.  */

static bool
is_signed_integer(const_tree var)
{
  return (TREE_CODE (TREE_TYPE (var)) == INTEGER_TYPE
	  && !TYPE_UNSIGNED (TREE_TYPE (var)));
}

/* If EXPR is an integer zero, return it.  If EXPR is SSA_NAME, and its
   defining statement is a subtraction of two signed integers, return the
   MINUS_EXPR.  Otherwise, return NULL_TREE.  */

static const_tree
prop_integer_zero_or_minus_expr (const_tree expr)
{
  if (integer_zerop (expr))
    return expr;

  if (TREE_CODE (expr) != SSA_NAME)
    return NULL_TREE;

  gimple *defining_stmt = SSA_NAME_DEF_STMT (expr);
  if (!is_gimple_assign (defining_stmt)
      || gimple_assign_rhs_code (defining_stmt) != MINUS_EXPR)
    return NULL_TREE;

  tree rhs1 = gimple_assign_rhs1 (defining_stmt);
  if (!is_signed_integer (rhs1))
    return NULL_TREE;

  tree rhs2 = gimple_assign_rhs2 (defining_stmt);
  if (!is_signed_integer (rhs2))
    return NULL_TREE;

  return build2 (MINUS_EXPR, TREE_TYPE (expr), rhs1, rhs2);
}

/* Optimize:

	1. (x - y) CMP 0
	2. 0 CMP (x - y)

   to:

	1. x CMP y
	2. y CMP x

   where CMP is either <, <=, >, or >=.  */

static void
optimize_signed_comparison (gcond *stmt)
{
  const enum tree_code code = gimple_cond_code (stmt);
  if (code < LT_EXPR || GE_EXPR < code)
    return;

  const_tree lhs = prop_integer_zero_or_minus_expr (gimple_cond_lhs (stmt));
  if (lhs == NULL_TREE)
    return;

  const_tree rhs = prop_integer_zero_or_minus_expr (gimple_cond_rhs (stmt));
  if (rhs == NULL_TREE)
    return;

  if (TREE_CODE (lhs) == MINUS_EXPR && integer_zerop (rhs))
    {
      /* Case 1: (x - y) CMP 0  */
      tree x = TREE_OPERAND (lhs, 0);
      tree y = TREE_OPERAND (lhs, 1);

      /* => x CMP y  */
      gimple_cond_set_lhs (stmt, x);
      gimple_cond_set_rhs (stmt, y);
      update_stmt (stmt);
    }
  else if (integer_zerop (lhs) && TREE_CODE (rhs) == MINUS_EXPR)
    {
      /* Case 2: 0 CMP (x - y)  */
      tree x = TREE_OPERAND (rhs, 0);
      tree y = TREE_OPERAND (rhs, 1);

      /* => y CMP x  */
      gimple_cond_set_lhs (stmt, y);
      gimple_cond_set_rhs (stmt, x);
      update_stmt (stmt);
    }
}

/* Find signed integer comparisons where the operands are MINUS_EXPR and
   0, and replace them with the equivalent comparison of the operands of
   the MINUS_EXPR without the subtraction.  */

namespace {

const pass_data pass_data_cmp =
{
  GIMPLE_PASS, /* type */
  "cmp", /* name */
  OPTGROUP_NONE, /* optinfo_flags */
  TV_TREE_CMP, /* tv_id */
  PROP_ssa, /* properties_required */
  0, /* properties_provided */
  0, /* properties_destroyed */
  0, /* todo_flags_start */
  TODO_remove_unused_locals, /* todo_flags_finish */
};

class pass_cmp : public gimple_opt_pass
{
public:
  pass_cmp (gcc::context *ctxt)
    : gimple_opt_pass (pass_data_cmp, ctxt)
  {}

  /* opt_pass methods: */
  bool gate (function *) final override
    {
      /* If -fwrapv is enabled, this pass cannot be safely applied as described
	 in the comment at the top of this file.  */
      return flag_tree_cmp != 0 && flag_wrapv == 0;
    }

  unsigned int execute (function *) final override;

}; // class pass_cmp

class cmp_dom_walker : public dom_walker
{
public:
  cmp_dom_walker () : dom_walker (CDI_DOMINATORS) {}

  void after_dom_children (basic_block) final override;
};

void
cmp_dom_walker::after_dom_children (basic_block bb)
{
  gimple_stmt_iterator gsi;

  for (gsi = gsi_after_labels (bb); !gsi_end_p (gsi); gsi_next (&gsi))
    {
      gimple *stmt = gsi_stmt (gsi);
      if (gimple_code (stmt) == GIMPLE_COND)
	optimize_signed_comparison (as_a <gcond *> (stmt));
    }
}


unsigned int
pass_cmp::execute (function *)
{
  calculate_dominance_info (CDI_DOMINATORS);
  cmp_dom_walker ().walk (ENTRY_BLOCK_PTR_FOR_FN (cfun));
  return 0;
}

} // anon namespace

gimple_opt_pass *
make_pass_cmp (gcc::context *ctxt)
{
  return new pass_cmp (ctxt);
}
