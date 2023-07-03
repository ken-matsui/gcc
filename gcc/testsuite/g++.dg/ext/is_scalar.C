// { dg-do compile { target c++11 } }

#define SA(X) static_assert((X),#X)

#define SA_TEST_FN(TRAIT, TYPE, EXPECT)		\
  SA(TRAIT(TYPE) == EXPECT);			\
  SA(TRAIT(const TYPE) == EXPECT)

#define SA_TEST_CATEGORY(TRAIT, TYPE, EXPECT)	\
  SA(TRAIT(TYPE) == EXPECT);			\
  SA(TRAIT(const TYPE) == EXPECT);		\
  SA(TRAIT(volatile TYPE) == EXPECT);		\
  SA(TRAIT(const volatile TYPE) == EXPECT)

class ClassType { };
enum EnumType { e0 };

SA_TEST_CATEGORY(__is_scalar, int, true);
SA_TEST_CATEGORY(__is_scalar, float, true);
SA_TEST_CATEGORY(__is_scalar, EnumType, true);
SA_TEST_CATEGORY(__is_scalar, int*, true);
SA_TEST_FN(__is_scalar, int(*)(int), true);
SA_TEST_CATEGORY(__is_scalar, int (ClassType::*), true);
SA_TEST_FN(__is_scalar, int (ClassType::*) (int), true);
SA_TEST_CATEGORY(__is_scalar, decltype(nullptr), true);

// Sanity check.
SA_TEST_CATEGORY(__is_scalar, ClassType, false);
