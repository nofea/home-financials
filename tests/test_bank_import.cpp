#include <gtest/gtest.h>
#include "test_helpers.hpp"
#include "home_manager.hpp"
#include "canara_bank_reader.hpp"
#include "bank_account.hpp"
#include <filesystem>
#include <fstream>
#include <memory>

// Fixture similar to HomeManagerTest but isolated for import tests
class BankImportTest : public TestDbFixture
{
protected:
    void SetUp() override
    {
        initHome("bank_import");
    }

    void TearDown() override
    {
        cleanup();
    }
    // Use TestDbFixture::home() and tmp_path
};

TEST_F(BankImportTest, CanaraCsvImportEndToEnd)
{
    // Setup family and member
    Family f("ImportFamily");
    ASSERT_EQ(home()->addFamily(f), commons::Result::Ok);

    Member m("Alice", "A");
    ASSERT_EQ(home()->addMemberToFamily(m, 1), commons::Result::Ok);

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
    auto res = home()->importBankStatement(reader, csv_path.string(), 1, std::string("Canara"), &inserted_id);
    EXPECT_EQ(res, commons::Result::Ok);
    EXPECT_NE(inserted_id, 0u);

    // Verify persisted row via StorageManager helper
    BankAccount row;
    auto* storage = home()->getStorageManager();
    auto gres = storage->getBankAccountById(inserted_id, &row);
    EXPECT_EQ(gres, commons::Result::Ok);
    EXPECT_EQ(row.getMemberId(), 1u);
    EXPECT_EQ(row.getAccountNumber(), "500012456");
    // Opening Rs.7,43,483.09 -> 74348309 paise
    EXPECT_EQ(row.getOpeningBalancePaise(), 74348309ll);
    // Closing Rs.9,99,999.00 -> 99999900 paise
    EXPECT_EQ(row.getClosingBalancePaise(), 99999900ll);

    // Cleanup sample file
    if (std::filesystem::exists(csv_path)) std::filesystem::remove(csv_path);
}
