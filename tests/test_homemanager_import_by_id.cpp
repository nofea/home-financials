#include <gtest/gtest.h>
#include "home_manager.hpp"
#include "family.hpp"
#include "member.hpp"
#include "canara_bank_reader.hpp"
#include <filesystem>
#include <fstream>

class HomeManagerImportByIdTest : public ::testing::Test
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

    std::filesystem::path tmp_path{std::filesystem::temp_directory_path() / "homefinancials_import_id_test.db"};
    std::unique_ptr<HomeManager> hm;
};

TEST_F(HomeManagerImportByIdTest, ImportUsingBankId)
{
    // Prepare family + member
    Family f("ImportByIdFamily");
    ASSERT_EQ(hm->addFamily(f), commons::Result::Ok);
    Member m("Carol", "C");
    ASSERT_EQ(hm->addMemberToFamily(m, 1), commons::Result::Ok);

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
    ASSERT_EQ(hm->getStorageManager()->getBankIdByName("Canara", &bank_id), commons::Result::Ok);

    CanaraBankReader reader;
    uint64_t inserted = 0;
    auto res = hm->importBankStatement(reader, csv_path.string(), 1, bank_id, &inserted);
    EXPECT_EQ(res, commons::Result::Ok);
    EXPECT_GT(inserted, 0u);

    // Verify via StorageManager
    StorageManager::BankAccountRow row;
    auto g = hm->getStorageManager()->getBankAccountById(inserted, &row);
    EXPECT_EQ(g, commons::Result::Ok);
    EXPECT_EQ(row.account_number, "500012456");
    EXPECT_EQ(row.opening_balance_paise, 27436909ll);
    EXPECT_EQ(row.closing_balance_paise, 74348309ll);

    std::filesystem::remove(csv_path);
}
