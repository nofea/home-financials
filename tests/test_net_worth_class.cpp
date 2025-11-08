#include <gtest/gtest.h>
#include "test_helpers.hpp"
#include "storage_manager.hpp"
#include "net_worth.hpp"
#include "family.hpp"
#include "member.hpp"
#include <filesystem>
#include <memory>

class NetWorthClassTest : public TestDbFixture
{
protected:
    void SetUp() override
    {
        initStorage("networth_class");
    }

    void TearDown() override
    {
        cleanup();
    }
    // Use TestDbFixture::tmp_path and storage()
};

TEST_F(NetWorthClassTest, MemberNetWorthSum)
{
    // Create family and a member
    Family family("NetFamilyClass");
    EXPECT_EQ(storage()->saveFamilyDataEx(family, nullptr), commons::Result::Ok);
    uint64_t family_id = storage()->listFamilies().front().getId();

    Member member("Alice", "A");
    uint64_t member_id = 0;
    EXPECT_EQ(storage()->saveMemberDataEx(member, family_id, &member_id), commons::Result::Ok);

    uint64_t bank_id = 0;
    EXPECT_EQ(storage()->getBankIdByName("Canara", &bank_id), commons::Result::Ok);

    EXPECT_EQ(storage()->saveBankAccountEx(bank_id, member_id, "ACC1", 10000, 15000, nullptr), commons::Result::Ok);
    EXPECT_EQ(storage()->saveBankAccountEx(bank_id, member_id, "ACC2", 5000, 25000, nullptr), commons::Result::Ok);

    NetWorth nw(storage());
    long long net_paise = 0;
    EXPECT_EQ(nw.computeMemberNetWorth(member_id, &net_paise), commons::Result::Ok);
    EXPECT_EQ(net_paise, 15000 + 25000);
}

TEST_F(NetWorthClassTest, FamilyNetWorthSum)
{
    Family family("FamilyTotalClass");
    EXPECT_EQ(storage()->saveFamilyDataEx(family, nullptr), commons::Result::Ok);
    uint64_t family_id = storage()->listFamilies().front().getId();

    Member m1("Bob", "B");
    Member m2("Carol", "C");
    uint64_t id1 = 0, id2 = 0;
    EXPECT_EQ(storage()->saveMemberDataEx(m1, family_id, &id1), commons::Result::Ok);
    EXPECT_EQ(storage()->saveMemberDataEx(m2, family_id, &id2), commons::Result::Ok);

    uint64_t bank_id = 0;
    EXPECT_EQ(storage()->getBankIdByName("SBI", &bank_id), commons::Result::Ok);

    EXPECT_EQ(storage()->saveBankAccountEx(bank_id, id1, "BACC", 0, 1000, nullptr), commons::Result::Ok);
    EXPECT_EQ(storage()->saveBankAccountEx(bank_id, id2, "CACC1", 0, 2000, nullptr), commons::Result::Ok);
    EXPECT_EQ(storage()->saveBankAccountEx(bank_id, id2, "CACC2", 0, 3000, nullptr), commons::Result::Ok);

    NetWorth nw(storage());
    long long family_paise = 0;
    EXPECT_EQ(nw.computeFamilyNetWorth(family_id, &family_paise), commons::Result::Ok);
    EXPECT_EQ(family_paise, 1000 + 2000 + 3000);
}

TEST_F(NetWorthClassTest, MemberNotFound)
{
    NetWorth nw(storage());
    long long out = 0;
    EXPECT_EQ(nw.computeMemberNetWorth(9999, &out), commons::Result::NotFound);
}
