#include <gtest/gtest.h>
#include "home_manager.hpp"
#include "family.hpp"
#include "member.hpp"
#include <filesystem>
#include <memory>

class HomeManagerNetWorthTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        tmp_path = std::filesystem::temp_directory_path() / "homefinancials_hm_networth_test.db";
        if (std::filesystem::exists(tmp_path))
        {
            std::filesystem::remove(tmp_path);
        }

        hm = std::make_unique<HomeManager>();
        auto* storage = hm->getStorageManager();
        ASSERT_TRUE(storage->initializeDatabase(tmp_path.string()));
    }

    void TearDown() override
    {
        hm.reset();
        if (std::filesystem::exists(tmp_path))
        {
            std::filesystem::remove(tmp_path);
        }
    }

    std::filesystem::path tmp_path;
    std::unique_ptr<HomeManager> hm;
};

TEST_F(HomeManagerNetWorthTest, MemberNetWorthSum)
{
    // Create family and add member via HomeManager
    Family family("NetFamilyHM");
    EXPECT_EQ(hm->addFamily(family), commons::Result::Ok);
    uint64_t family_id = hm->listFamilies().front().getId();

    Member member("Alice", "A");
    EXPECT_EQ(hm->addMemberToFamily(member, family_id), commons::Result::Ok);
    uint64_t member_id = hm->listMembersOfFamily(family_id).front().getId();

    // Resolve a bank id and add accounts using StorageManager (HomeManager doesn't expose account saves)
    auto* storage = hm->getStorageManager();
    uint64_t bank_id = 0;
    EXPECT_EQ(storage->getBankIdByName("Canara", &bank_id), commons::Result::Ok);

    // Insert two bank accounts for the member with known closing balances
    EXPECT_EQ(storage->saveBankAccountEx(bank_id, member_id, "ACC1", 10000, 15000, nullptr), commons::Result::Ok);
    EXPECT_EQ(storage->saveBankAccountEx(bank_id, member_id, "ACC2", 5000, 25000, nullptr), commons::Result::Ok);

    long long net_paise = 0;
    EXPECT_EQ(hm->computeMemberNetWorth(member_id, &net_paise), commons::Result::Ok);
    EXPECT_EQ(net_paise, 15000 + 25000);
}

TEST_F(HomeManagerNetWorthTest, FamilyNetWorthSum)
{
    // Create family
    Family family("FamilyTotalHM");
    EXPECT_EQ(hm->addFamily(family), commons::Result::Ok);
    uint64_t family_id = hm->listFamilies().front().getId();

    // Add two members
    Member m1("Bob", "B");
    Member m2("Carol", "C");
    EXPECT_EQ(hm->addMemberToFamily(m1, family_id), commons::Result::Ok);
    EXPECT_EQ(hm->addMemberToFamily(m2, family_id), commons::Result::Ok);

    auto members = hm->listMembersOfFamily(family_id);
    ASSERT_EQ(members.size(), 2);
    uint64_t id1 = members[0].getId();
    uint64_t id2 = members[1].getId();

    auto* storage = hm->getStorageManager();
    uint64_t bank_id = 0;
    EXPECT_EQ(storage->getBankIdByName("SBI", &bank_id), commons::Result::Ok);

    // Add accounts: member1 -> 1000 paise, member2 -> 2000 + 3000 paise
    EXPECT_EQ(storage->saveBankAccountEx(bank_id, id1, "BACC", 0, 1000, nullptr), commons::Result::Ok);
    EXPECT_EQ(storage->saveBankAccountEx(bank_id, id2, "CACC1", 0, 2000, nullptr), commons::Result::Ok);
    EXPECT_EQ(storage->saveBankAccountEx(bank_id, id2, "CACC2", 0, 3000, nullptr), commons::Result::Ok);

    long long family_paise = 0;
    EXPECT_EQ(hm->computeFamilyNetWorth(family_id, &family_paise), commons::Result::Ok);
    EXPECT_EQ(family_paise, 1000 + 2000 + 3000);
}

TEST_F(HomeManagerNetWorthTest, MemberNotFound)
{
    long long out = 0;
    EXPECT_EQ(hm->computeMemberNetWorth(9999, &out), commons::Result::NotFound);
}

// main() intentionally omitted so this file can be linked into the
// existing test_home_manager executable which provides the test
// runner's main (gtest_main).
