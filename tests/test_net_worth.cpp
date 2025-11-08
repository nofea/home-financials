#include <gtest/gtest.h>
#include "storage_manager.hpp"
#include "net_worth.hpp"
#include "family.hpp"
#include "member.hpp"
#include <filesystem>
#include <memory>


class NetWorthTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        tmp_path = std::filesystem::temp_directory_path() / "homefinancials_networth_test.db";
        if (std::filesystem::exists(tmp_path))
        {
            std::filesystem::remove(tmp_path);
        }

        sm = std::make_unique<StorageManager>();
        ASSERT_TRUE(sm->initializeDatabase(tmp_path.string()));
    }

    void TearDown() override
    {
        sm.reset();
        if (std::filesystem::exists(tmp_path))
        {
            std::filesystem::remove(tmp_path);
        }
    }

    std::filesystem::path tmp_path;
    std::unique_ptr<StorageManager> sm;
};


TEST_F(NetWorthTest, MemberNetWorthSum)
{
    // Create family and a member
    Family family("NetFamily");
    EXPECT_EQ(sm->saveFamilyDataEx(family, nullptr), commons::Result::Ok);
    uint64_t family_id = sm->listFamilies().front().getId();

    Member member("Alice", "A");
    uint64_t member_id = 0;
    EXPECT_EQ(sm->saveMemberDataEx(member, family_id, &member_id), commons::Result::Ok);

    // Resolve a bank id
    uint64_t bank_id = 0;
    EXPECT_EQ(sm->getBankIdByName("Canara", &bank_id), commons::Result::Ok);

    // Insert two bank accounts for the member with known closing balances
    EXPECT_EQ(sm->saveBankAccountEx(bank_id, member_id, "ACC1", 10000, 15000, nullptr), commons::Result::Ok);
    EXPECT_EQ(sm->saveBankAccountEx(bank_id, member_id, "ACC2", 5000, 25000, nullptr), commons::Result::Ok);

    NetWorth nw(sm.get());
    long long net_paise = 0;
    EXPECT_EQ(nw.computeMemberNetWorth(member_id, &net_paise), commons::Result::Ok);
    // closing balances: 15000 + 25000 = 40000 paise
    EXPECT_EQ(net_paise, 15000 + 25000);
}


TEST_F(NetWorthTest, FamilyNetWorthSum)
{
    // Create family
    Family family("FamilyTotal");
    EXPECT_EQ(sm->saveFamilyDataEx(family, nullptr), commons::Result::Ok);
    uint64_t family_id = sm->listFamilies().front().getId();

    // Add two members
    Member m1("Bob", "B");
    Member m2("Carol", "C");
    uint64_t id1 = 0, id2 = 0;
    EXPECT_EQ(sm->saveMemberDataEx(m1, family_id, &id1), commons::Result::Ok);
    EXPECT_EQ(sm->saveMemberDataEx(m2, family_id, &id2), commons::Result::Ok);

    uint64_t bank_id = 0;
    EXPECT_EQ(sm->getBankIdByName("SBI", &bank_id), commons::Result::Ok);

    // Add accounts: member1 -> 1000 paise, member2 -> 2000 + 3000 paise
    EXPECT_EQ(sm->saveBankAccountEx(bank_id, id1, "BACC", 0, 1000, nullptr), commons::Result::Ok);
    EXPECT_EQ(sm->saveBankAccountEx(bank_id, id2, "CACC1", 0, 2000, nullptr), commons::Result::Ok);
    EXPECT_EQ(sm->saveBankAccountEx(bank_id, id2, "CACC2", 0, 3000, nullptr), commons::Result::Ok);

    NetWorth nw(sm.get());
    long long family_paise = 0;
    EXPECT_EQ(nw.computeFamilyNetWorth(family_id, &family_paise), commons::Result::Ok);
    EXPECT_EQ(family_paise, 1000 + 2000 + 3000);
}


TEST_F(NetWorthTest, MemberNotFound)
{
    NetWorth nw(sm.get());
    long long out = 0;
    EXPECT_EQ(nw.computeMemberNetWorth(9999, &out), commons::Result::NotFound);
}
