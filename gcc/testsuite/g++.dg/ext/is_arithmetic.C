// { dg-do compile { target c++11 } }

#define SA(X) static_assert((X),#X)

#define SA_TEST_CATEGORY(TRAIT, TYPE, EXPECT)	\
  SA(TRAIT(TYPE) == EXPECT);			\
  SA(TRAIT(const TYPE) == EXPECT);		\
  SA(TRAIT(volatile TYPE) == EXPECT);		\
  SA(TRAIT(const volatile TYPE) == EXPECT)

SA_TEST_CATEGORY(__is_arithmetic, void, false);

SA_TEST_CATEGORY(__is_arithmetic, char, true);
SA_TEST_CATEGORY(__is_arithmetic, signed char, true);
SA_TEST_CATEGORY(__is_arithmetic, unsigned char, true);
SA_TEST_CATEGORY(__is_arithmetic, wchar_t, true);
SA_TEST_CATEGORY(__is_arithmetic, short, true);
SA_TEST_CATEGORY(__is_arithmetic, unsigned short, true);
SA_TEST_CATEGORY(__is_arithmetic, int, true);
SA_TEST_CATEGORY(__is_arithmetic, unsigned int, true);
SA_TEST_CATEGORY(__is_arithmetic, long, true);
SA_TEST_CATEGORY(__is_arithmetic, unsigned long, true);
SA_TEST_CATEGORY(__is_arithmetic, long long, true);
SA_TEST_CATEGORY(__is_arithmetic, unsigned long long, true);
SA_TEST_CATEGORY(__is_arithmetic, float, true);
SA_TEST_CATEGORY(__is_arithmetic, double, true);
SA_TEST_CATEGORY(__is_arithmetic, long double, true);

// Sanity check.
class ClassType { };
SA_TEST_CATEGORY(__is_arithmetic, ClassType, false);
