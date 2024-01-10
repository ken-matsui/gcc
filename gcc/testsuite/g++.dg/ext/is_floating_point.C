// { dg-do compile { target c++11 } }

#define SA(X) static_assert((X),#X)

#define SA_TEST_CATEGORY(TRAIT, TYPE, EXPECT)	\
  SA(TRAIT(TYPE) == EXPECT);			\
  SA(TRAIT(const TYPE) == EXPECT);		\
  SA(TRAIT(volatile TYPE) == EXPECT);		\
  SA(TRAIT(const volatile TYPE) == EXPECT)

SA_TEST_CATEGORY(__is_floating_point, void, false);
SA_TEST_CATEGORY(__is_floating_point, char, false);
SA_TEST_CATEGORY(__is_floating_point, signed char, false);
SA_TEST_CATEGORY(__is_floating_point, unsigned char, false);
SA_TEST_CATEGORY(__is_floating_point, wchar_t, false);
SA_TEST_CATEGORY(__is_floating_point, short, false);
SA_TEST_CATEGORY(__is_floating_point, unsigned short, false);
SA_TEST_CATEGORY(__is_floating_point, int, false);
SA_TEST_CATEGORY(__is_floating_point, unsigned int, false);
SA_TEST_CATEGORY(__is_floating_point, long, false);
SA_TEST_CATEGORY(__is_floating_point, unsigned long, false);
SA_TEST_CATEGORY(__is_floating_point, long long, false);
SA_TEST_CATEGORY(__is_floating_point, unsigned long long, false);

SA_TEST_CATEGORY(__is_floating_point, float, true);
SA_TEST_CATEGORY(__is_floating_point, double, true);
SA_TEST_CATEGORY(__is_floating_point, long double, true);

#ifndef __STRICT_ANSI__
// GNU Extensions.
#ifdef _GLIBCXX_USE_FLOAT128
SA_TEST_CATEGORY(__is_floating_point, __float128, true);
#endif

#ifdef __SIZEOF_INT128__
SA_TEST_CATEGORY(__is_floating_point, __int128, false);
SA_TEST_CATEGORY(__is_floating_point, unsigned __int128, false);
#endif
#endif

// Sanity check.
class ClassType { };
SA_TEST_CATEGORY(__is_floating_point, ClassType, false);
