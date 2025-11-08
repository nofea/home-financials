#include <gtest/gtest.h>
#include "canara_bank_reader.hpp"
#include <filesystem>
#include <fstream>

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
