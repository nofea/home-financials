#include "bank_account.hpp"
#include <gtest/gtest.h>

TEST(BankAccountTest, PaiseToRupees)
{
    EXPECT_DOUBLE_EQ(BankAccount::paiseToRupees(0), 0.0);
    EXPECT_DOUBLE_EQ(BankAccount::paiseToRupees(100), 1.0);
    EXPECT_DOUBLE_EQ(BankAccount::paiseToRupees(-150), -1.5);
}

TEST(BankAccountTest, GetBalanceRupees)
{
    BankAccount bankeB(1, 2, 3, "ACC123", 12345, 67890);
    EXPECT_DOUBLE_EQ(bankeB.getOpeningBalanceRupees(), 123.45);
    EXPECT_DOUBLE_EQ(bankeB.getClosingBalanceRupees(), 678.90);
}

TEST(BankAccountTest, ToStringContainsFields)
{
    BankAccount bankeB(10, 20, 30, "ABC-123 456", 100, 200);
    std::string str = bankeB.toString();
    EXPECT_NE(str.find("id=10"), std::string::npos);
    EXPECT_NE(str.find("bank_id=20"), std::string::npos);
    EXPECT_NE(str.find("member_id=30"), std::string::npos);
    EXPECT_NE(str.find("account='ABC-123 456'"), std::string::npos);
}

TEST(BankAccountTest, NormalizeAccountNumber)
{
    EXPECT_EQ(BankAccount::normalizeAccountNumber("abc 123-xyz"), "ABC123XYZ");
    EXPECT_EQ(BankAccount::normalizeAccountNumber("  a - b - c "), "ABC");
    EXPECT_EQ(BankAccount::normalizeAccountNumber("Acc\tNum-99"), "ACCNUM99");
}

TEST(BankAccountTest, ValueEqualityByAccessors)
{
    BankAccount bankA(1, 2, 3, "001", 500, 600);
    BankAccount bankB(1, 2, 3, "001", 500, 600);

    EXPECT_TRUE(bankA == bankB);
    EXPECT_FALSE(bankA != bankB);
}

TEST(BankAccountTest, InequalityDifferentBalances)
{
    BankAccount bankC(1, 2, 3, "001", 500, 600);
    BankAccount bankD(1, 2, 3, "001", 500, 601);
    EXPECT_FALSE(bankC == bankD);
    EXPECT_TRUE(bankC != bankD);
}
