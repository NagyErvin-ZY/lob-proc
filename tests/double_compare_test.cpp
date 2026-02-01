#include "test_common.h"

TEST(SafeDoubleCompareTest, EqualValuesReturnTrue) {
    double a = 100.0, b = 100.0;
    EXPECT_TRUE(SafeDoubleCompare(a, b));
}

TEST(SafeDoubleCompareTest, ValuesWithinEpsilonReturnTrue) {
    double a = 100.0, b = 100.0 + DoubleComparisonEpsilon / 2;
    EXPECT_TRUE(SafeDoubleCompare(a, b));
}

TEST(SafeDoubleCompareTest, ValuesOutsideEpsilonReturnFalse) {
    double a = 100.0, b = 100.0 + DoubleComparisonEpsilon * 2;
    EXPECT_FALSE(SafeDoubleCompare(a, b));
}

TEST(SafeDoubleCompareTest, NegativeValuesWithinEpsilon) {
    double a = -100.0, b = -100.0 + DoubleComparisonEpsilon / 2;
    EXPECT_TRUE(SafeDoubleCompare(a, b));
}

TEST(SafeDoubleCompareTest, ZeroComparison) {
    double a = 0.0, b = 0.0;
    EXPECT_TRUE(SafeDoubleCompare(a, b));
}

TEST(CustomComparatorTest, ComparatorWithDefaultEpsilon) {
    CustomDoubleComparator comp;
    EXPECT_FALSE(comp(100.0, 100.0));
    EXPECT_FALSE(comp(100.0, 100.0 + DoubleComparisonEpsilon / 2));
    EXPECT_TRUE(comp(100.0, 100.0 + DoubleComparisonEpsilon * 2));
    EXPECT_FALSE(comp(100.0 + DoubleComparisonEpsilon * 2, 100.0));
}

TEST(CustomComparatorTest, ComparatorWithCustomEpsilon) {
    CustomDoubleComparator comp(0.1);
    EXPECT_FALSE(comp(100.0, 100.05));
    EXPECT_TRUE(comp(100.0, 100.2));
}
