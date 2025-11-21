#include <gtest/gtest.h>
#include "test_helpers.hpp"
#include "canara_bank_reader.hpp"
#include "home_manager.hpp"
#include <filesystem>
#include <fstream>

class IntegrationImportTest : public TestDbFixture
{
protected:
    void SetUp() override
    {
        initHome("integration");
    }

    void TearDown() override
    {
        cleanup();
    }
};

TEST_F(IntegrationImportTest, ImportStatementsAndComputeFamilyNetWorth)
{
    // Create family and two members using HomeManager
    Family family("IntegrationFamily");
    ASSERT_EQ(home()->addFamily(family), commons::Result::Ok);
    uint64_t family_id = home()->listFamilies().front().getId();

    Member m1("Alice", "A");
    Member m2("Bob", "B");
    uint64_t id1 = 0, id2 = 0;
    ASSERT_EQ(home()->addMemberToFamily(m1, family_id, &id1), commons::Result::Ok);
    ASSERT_EQ(home()->addMemberToFamily(m2, family_id, &id2), commons::Result::Ok);

    // Create two small Canara-like CSV samples
    auto csv1 = std::filesystem::temp_directory_path() / "int_canara1.csv";
    auto csv2 = std::filesystem::temp_directory_path() / "int_canara2.csv";

    {
        std::ofstream ofs(csv1);
        ofs << "Account Number,=\"500012456\"\n";
        ofs << "Opening Balance,\"Rs.7,43,483.09\"\n";
        ofs << "Closing Balance,\"Rs.9,99,999.00\"\n";
    }

    {
        std::ofstream ofs(csv2);
        ofs << "Account Number,=\"600012456\"\n";
        ofs << "Opening Balance,\"Rs.1,00,000.00\"\n";
        ofs << "Closing Balance,\"Rs.2,50,000.00\"\n";
    }

    // Import via HomeManager convenience overload (by bank name)
    uint64_t acc1 = 0, acc2 = 0;
    ASSERT_EQ(home()->importBankStatement(csv1.string(), id1, std::string("Canara"), &acc1), commons::Result::Ok);
    ASSERT_NE(acc1, 0u);

    ASSERT_EQ(home()->importBankStatement(csv2.string(), id2, std::string("Canara"), &acc2), commons::Result::Ok);
    ASSERT_NE(acc2, 0u);

    // Compute family net worth and assert sum of closing balances
    long long family_paise = 0;
    ASSERT_EQ(home()->computeFamilyNetWorth(family_id, &family_paise), commons::Result::Ok);

    // Expected: 99999900 + 25000000 = 124999900 paise
    EXPECT_EQ(family_paise, 99999900ll + 25000000ll);

    // Cleanup temporary files
    if (std::filesystem::exists(csv1)) std::filesystem::remove(csv1);
    if (std::filesystem::exists(csv2)) std::filesystem::remove(csv2);
}
