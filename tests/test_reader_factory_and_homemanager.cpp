#include <gtest/gtest.h>
#include "home_manager.hpp"
#include "family.hpp"
#include "member.hpp"
#include <filesystem>
#include <fstream>

class ReaderFactoryHomeManagerTest : public ::testing::Test
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

    std::filesystem::path tmp_path{std::filesystem::temp_directory_path() / "homefinancials_factory_test.db"};
    std::unique_ptr<HomeManager> hm;
};

TEST_F(ReaderFactoryHomeManagerTest, ImportByNameAndById)
{
    // Setup family + member
    Family f("FactoryFamily");
    ASSERT_EQ(hm->addFamily(f), commons::Result::Ok);
    Member m("Daisy", "D");
    ASSERT_EQ(hm->addMemberToFamily(m, 1), commons::Result::Ok);

    // Create sample CSV
    auto csv_path = std::filesystem::temp_directory_path() / "canara_factory.csv";
    {
        std::ofstream ofs(csv_path);
        ofs << "Account Number,=\"500012456   \"\n";
        ofs << "Opening Balance,\"Rs.2,74,369.09\"\n";
        ofs << "Closing Balance,\"Rs.7,43,483.09\"\n";
    }

    // Import by bank name
    uint64_t inserted1 = 0;
    auto res1 = hm->importBankStatement(csv_path.string(), 1, std::string("Canara"), &inserted1);
    EXPECT_EQ(res1, commons::Result::Ok);
    EXPECT_GT(inserted1, 0u);

    StorageManager::BankAccountRow row1;
    EXPECT_EQ(hm->getStorageManager()->getBankAccountById(inserted1, &row1), commons::Result::Ok);
    EXPECT_EQ(row1.account_number, "500012456");

    // Resolve bank id and import again by id
    uint64_t bank_id = 0;
    ASSERT_EQ(hm->getStorageManager()->getBankIdByName("Canara", &bank_id), commons::Result::Ok);

    uint64_t inserted2 = 0;
    auto res2 = hm->importBankStatement(csv_path.string(), 1, bank_id, &inserted2);
    EXPECT_EQ(res2, commons::Result::Ok);
    EXPECT_GT(inserted2, 0u);

    StorageManager::BankAccountRow row2;
    EXPECT_EQ(hm->getStorageManager()->getBankAccountById(inserted2, &row2), commons::Result::Ok);
    EXPECT_EQ(row2.account_number, "500012456");

    std::filesystem::remove(csv_path);
}
