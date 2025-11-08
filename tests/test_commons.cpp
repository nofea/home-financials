#include <gtest/gtest.h>
#include "commons.hpp"

TEST(ParseMoneyToPaise, BasicFormats)
{
    auto v1 = commons::parseMoneyToPaise("Rs.7,43,483.09");
    ASSERT_TRUE(v1.has_value());
    EXPECT_EQ(*v1, 74348309ll);

    auto v2 = commons::parseMoneyToPaise("3,23,527.09");
    ASSERT_TRUE(v2.has_value());
    EXPECT_EQ(*v2, 32352709ll);

    auto v3 = commons::parseMoneyToPaise("999.5");
    ASSERT_TRUE(v3.has_value());
    EXPECT_EQ(*v3, 99950ll);
}

TEST(ParseMoneyToPaise, NegativeAndMalformed)
{
    auto neg = commons::parseMoneyToPaise("-1,234.50");
    ASSERT_TRUE(neg.has_value());
    EXPECT_EQ(*neg, -123450ll);

    auto bad = commons::parseMoneyToPaise("not a number");
    EXPECT_FALSE(bad.has_value());
}
