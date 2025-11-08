#include <gtest/gtest.h>
#include "home_manager.hpp"
#include "canara_bank_reader.hpp"
#include <filesystem>
#include <fstream>
#include <memory>

// Fixture similar to HomeManagerTest but isolated for import tests
class BankImportTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        if (std::filesystem::exists(tmp_path))
        {
            std::filesystem::remove(tmp_path);
        }
        hm = std::make_unique<HomeManager>();
        auto* storage = hm->getStorageManager();
        storage->initializeDatabase(tmp_path.string());
    }

    void TearDown() override
    {
        hm.reset();
        if (std::filesystem::exists(tmp_path))
        {
            std::filesystem::remove(tmp_path);
        }
    }

    std::filesystem::path tmp_path{std::filesystem::temp_directory_path() / "homefinancials_import_test.db"};
    std::unique_ptr<HomeManager> hm;
};

TEST_F(BankImportTest, CanaraCsvImportEndToEnd)
{
    // Setup family and member
    Family f("ImportFamily");
    ASSERT_EQ(hm->addFamily(f), commons::Result::Ok);

    Member m("Alice", "A");
    ASSERT_EQ(hm->addMemberToFamily(m, 1), commons::Result::Ok);

    // Create a small Canara-like CSV sample
    auto csv_path = std::filesystem::temp_directory_path() / "canara_sample.csv";
    {
        std::ofstream ofs(csv_path);
        ofs << "Account Number,=\"500012456\"\n";
        ofs << "Opening Balance,\"Rs.7,43,483.09\"\n";
        ofs << "Closing Balance,\"Rs.9,99,999.00\"\n";
    }

    CanaraBankReader reader;
    uint64_t inserted_id = 0;
    auto res = hm->importBankStatement(reader, csv_path.string(), 1, std::string("Canara"), &inserted_id);
    EXPECT_EQ(res, commons::Result::Ok);
    EXPECT_NE(inserted_id, 0u);

    // Verify persisted row via StorageManager helper
    StorageManager::BankAccountRow row;
    auto* storage = hm->getStorageManager();
    auto gres = storage->getBankAccountById(inserted_id, &row);
    EXPECT_EQ(gres, commons::Result::Ok);
    EXPECT_EQ(row.member_id, 1u);
    EXPECT_EQ(row.account_number, "500012456");
    // Opening Rs.7,43,483.09 -> 74348309 paise
    EXPECT_EQ(row.opening_balance_paise, 74348309ll);
    // Closing Rs.9,99,999.00 -> 99999900 paise
    EXPECT_EQ(row.closing_balance_paise, 99999900ll);

    // Cleanup sample file
    if (std::filesystem::exists(csv_path)) std::filesystem::remove(csv_path);
}
