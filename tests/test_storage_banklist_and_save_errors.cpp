#include <gtest/gtest.h>
#include "home_manager.hpp"
#include "family.hpp"
#include "member.hpp"
#include <filesystem>

class StorageBankListTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        if (std::filesystem::exists(tmp_path)) std::filesystem::remove(tmp_path);
        hm = std::make_unique<HomeManager>();
        hm->getStorageManager()->initializeDatabase(tmp_path.string());
    }

    void TearDown() override
    {
        hm.reset();
        if (std::filesystem::exists(tmp_path)) std::filesystem::remove(tmp_path);
    }

    std::filesystem::path tmp_path{std::filesystem::temp_directory_path() / "homefinancials_storage_test.db"};
    std::unique_ptr<HomeManager> hm;
};

TEST_F(StorageBankListTest, BankListPrepopulatedAndCaseInsensitive)
{
    uint64_t id1 = 0;
    auto res = hm->getStorageManager()->getBankIdByName("Canara", &id1);
    EXPECT_EQ(res, commons::Result::Ok);
    EXPECT_GT(id1, 0u);

    uint64_t id2 = 0;
    auto res2 = hm->getStorageManager()->getBankIdByName("canara", &id2);
    EXPECT_EQ(res2, commons::Result::Ok);
    EXPECT_EQ(id1, id2);
}

TEST_F(StorageBankListTest, SaveBankAccountErrorsWhenMissingRefs)
{
    // Create a family and member so we can test missing bank and missing member
    Family f("F1");
    ASSERT_EQ(hm->addFamily(f), commons::Result::Ok);
    Member m("Bob", "B");
    uint64_t member_id = 0;
    ASSERT_EQ(hm->addMemberToFamily(m, 1, &member_id), commons::Result::Ok);
    EXPECT_GT(member_id, 0u);

    // Use an invalid bank id - should return NotFound
    auto s = hm->getStorageManager();
    auto res = s->saveBankAccountEx(999999, member_id, "acc", 1000ll, 2000ll, nullptr);
    EXPECT_EQ(res, commons::Result::NotFound);

    // Use an invalid member id - should return NotFound
    uint64_t bank_id = 0;
    ASSERT_EQ(s->getBankIdByName("Canara", &bank_id), commons::Result::Ok);
    auto res2 = s->saveBankAccountEx(bank_id, 999999, "acc", 1000ll, 2000ll, nullptr);
    EXPECT_EQ(res2, commons::Result::NotFound);
}
