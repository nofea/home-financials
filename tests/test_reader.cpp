#include <gtest/gtest.h>
#include "test_helpers.hpp"
#include "canara_bank_reader.hpp"
#include "home_manager.hpp"
#include "family.hpp"
#include "member.hpp"
#include <filesystem>
#include <fstream>
#include <memory>

TEST(CanaraBankReader, ExtractBeforeAndAfterParse)
{
    CanaraBankReader reader;

    // Before parsing, extractor should be empty
    EXPECT_EQ(reader.extractAccountInfo(), std::nullopt);

    // Create a small sample file
    auto csv_path = std::filesystem::temp_directory_path() / "canara_reader_test.csv";
    {
        std::ofstream ofs(csv_path);
        ofs << "Account Number,=\"500012456   \"\n";
        ofs << "Opening Balance,\"Rs.2,74,369.09\"\n";
        ofs << "Closing Balance,\"Rs.7,43,483.09\"\n";
    }

    // parseFile() should open and parse the file successfully
    auto r = reader.parseFile(csv_path.string());
    EXPECT_EQ(r, commons::Result::Ok);

    auto info = reader.extractAccountInfo();
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->accountNumber, "500012456");
    EXPECT_EQ(info->openingBalancePaise, 27436909ll);
    EXPECT_EQ(info->closingBalancePaise, 74348309ll);

    // parseFile on missing file returns NotFound
    CanaraBankReader r2;
    auto rres = r2.parseFile("/nonexistent/file/does_not_exist.csv");
    EXPECT_EQ(rres, commons::Result::NotFound);

    std::filesystem::remove(csv_path);
}


class ReaderFactoryHomeManagerTest : public TestDbFixture
{
protected:
    void SetUp() override
    {
        initHome("reader_factory");
    }

    void TearDown() override
    {
        cleanup();
    }
    // Use TestDbFixture::home() and tmp_path
};

TEST_F(ReaderFactoryHomeManagerTest, ImportByNameAndById)
{
    // Setup family + member
    Family f("FactoryFamily");
    ASSERT_EQ(home()->addFamily(f), commons::Result::Ok);
    Member m("Daisy", "D");
    ASSERT_EQ(home()->addMemberToFamily(m, 1), commons::Result::Ok);

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
    auto res1 = home()->importBankStatement(csv_path.string(), 1, std::string("Canara"), &inserted1);
    EXPECT_EQ(res1, commons::Result::Ok);
    EXPECT_GT(inserted1, 0u);

    StorageManager::BankAccountRow row1;
    EXPECT_EQ(home()->getStorageManager()->getBankAccountById(inserted1, &row1), commons::Result::Ok);
    EXPECT_EQ(row1.account_number, "500012456");

    // Resolve bank id and import again by id
    uint64_t bank_id = 0;
    ASSERT_EQ(home()->getStorageManager()->getBankIdByName("Canara", &bank_id), commons::Result::Ok);

    uint64_t inserted2 = 0;
    auto res2 = home()->importBankStatement(csv_path.string(), 1, bank_id, &inserted2);
    EXPECT_EQ(res2, commons::Result::Ok);
    EXPECT_GT(inserted2, 0u);

    StorageManager::BankAccountRow row2;
    EXPECT_EQ(home()->getStorageManager()->getBankAccountById(inserted2, &row2), commons::Result::Ok);
    EXPECT_EQ(row2.account_number, "500012456");

    std::filesystem::remove(csv_path);
}

// Import-by-id test consolidated here (previously in test_homemanager_import_by_id.cpp)
class HomeManagerImportByIdTest : public TestDbFixture
{
protected:
    void SetUp() override
    {
        initHome("reader_import_id");
    }

    void TearDown() override
    {
        cleanup();
    }
    // Use TestDbFixture::home() and tmp_path
};

TEST_F(HomeManagerImportByIdTest, ImportUsingBankId)
{
    // Prepare family + member
    Family f("ImportByIdFamily");
    ASSERT_EQ(home()->addFamily(f), commons::Result::Ok);
    Member m("Carol", "C");
    ASSERT_EQ(home()->addMemberToFamily(m, 1), commons::Result::Ok);

    // Write sample CSV
    auto csv_path = std::filesystem::temp_directory_path() / "canara_import_by_id.csv";
    {
        std::ofstream ofs(csv_path);
        ofs << "Account Number,=\"500012456   \"\n";
        ofs << "Opening Balance,\"Rs.2,74,369.09\"\n";
        ofs << "Closing Balance,\"Rs.7,43,483.09\"\n";
    }

    // Resolve bank id for Canara
    uint64_t bank_id = 0;
    ASSERT_EQ(home()->getStorageManager()->getBankIdByName("Canara", &bank_id), commons::Result::Ok);

    CanaraBankReader reader;
    uint64_t inserted = 0;
    auto res = home()->importBankStatement(reader, csv_path.string(), 1, bank_id, &inserted);
    EXPECT_EQ(res, commons::Result::Ok);
    EXPECT_GT(inserted, 0u);

    // Verify via StorageManager
    StorageManager::BankAccountRow row;
    auto g = home()->getStorageManager()->getBankAccountById(inserted, &row);
    EXPECT_EQ(g, commons::Result::Ok);
    EXPECT_EQ(row.account_number, "500012456");
    EXPECT_EQ(row.opening_balance_paise, 27436909ll);
    EXPECT_EQ(row.closing_balance_paise, 74348309ll);

    std::filesystem::remove(csv_path);
}
