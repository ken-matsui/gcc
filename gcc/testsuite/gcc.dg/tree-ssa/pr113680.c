/* { dg-do compile } */
/* { dg-options "-Os -fdump-tree-optimized" } */

volatile int i = 0;

void foo() { i = 1; }
void bar() { i = 2; }

void f1(int x, int y)
{
  int diff = x - y;
  if (diff > 0)
    foo();
  if (diff < 0)
    bar();
}

void f2(int x, int y)
{
  if ((x - y) > 0)
    foo();
  if ((x - y) < 0)
    bar();
}

void f3(int x, int y)
{
  if (x > y)
    foo();
  if (x < y)
    bar();
}

void f4(int x, int y)
{
  int diff = x - y;
  if (diff > 0)
    foo();
  if (x < y)
    bar();
}

/* { dg-final { scan-tree-dump-not " - " "optimized" } } */
/* { dg-final { scan-assembler-times "cmp" 1 { target { i?86-*-* x86_64-*-* } } } } */
/* { dg-final { scan-assembler-times "jmp\[ \t\]+f1" 3 { target { i?86-*-* x86_64-*-* } } } } */
/* { dg-final { scan-assembler-not "sub" { target { i?86-*-* x86_64-*-* } } } } */
/* { dg-final { scan-assembler-not "test" { target { i?86-*-* x86_64-*-* } } } } */
