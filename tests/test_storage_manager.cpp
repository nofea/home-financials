#include <gtest/gtest.h>
#include "storage_manager.hpp"
#include "family.hpp"
#include "member.hpp"
#include <filesystem>
#include <sqlite3.h>
#include <memory>


/**
 * Test fixture for StorageManager tests.
 *
 * Creates a temporary database file for each test and removes it on teardown.
 */
class StorageManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        tmp_path = std::filesystem::temp_directory_path() / "homefinancials_test.db";
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

    /**
     * Return the row count for a table in the temporary DB.
     */
    int getTableRowCount(const std::string &tableName)
    {
        sqlite3 *db = nullptr;
        int count = 0;
        int ret_code = sqlite3_open(tmp_path.string().c_str(), &db);
        EXPECT_EQ(ret_code, SQLITE_OK);

        std::string sql = "SELECT COUNT(*) FROM " + tableName + ";";
        sqlite3_stmt *stmt = nullptr;
        ret_code = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
        EXPECT_EQ(ret_code, SQLITE_OK);

        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            count = sqlite3_column_int(stmt, 0);
        }

        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return count;
    }

    /**
     * Return the largest rowid for a table (useful to find last-inserted id).
     */
    int64_t getLastInsertId(const std::string &tableName)
    {
        sqlite3 *db = nullptr;
        int64_t id = 0;
        int ret_code = sqlite3_open(tmp_path.string().c_str(), &db);
        EXPECT_EQ(ret_code, SQLITE_OK);

        std::string sql = "SELECT MAX(rowid) FROM " + tableName + ";";
        sqlite3_stmt *stmt = nullptr;
        ret_code = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
        EXPECT_EQ(ret_code, SQLITE_OK);

        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            id = sqlite3_column_int64(stmt, 0);
        }

        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return id;
    }

    std::filesystem::path tmp_path;
    std::unique_ptr<StorageManager> sm;
};

TEST_F(StorageManagerTest, InitializeCreatesDBAndTables)
{
    namespace fs = std::filesystem;
    // create a temp path under system temp
    fs::path tmp = fs::temp_directory_path() / "homefinancials_test.db";
    if (fs::exists(tmp)) fs::remove(tmp);

    StorageManager sm;
    // Initialize with explicit path
    bool ok = sm.initializeDatabase(tmp.string());
    EXPECT_TRUE(ok);

    // Open sqlite and verify tables exist
    sqlite3* db = nullptr;
    int ret_code = sqlite3_open(tmp.string().c_str(), &db);
    EXPECT_EQ(ret_code, SQLITE_OK);
    
    const char* sql = "SELECT name FROM sqlite_master WHERE type='table' AND name IN ('FamilyInfo','MemberInfo');";
    sqlite3_stmt* stmt = nullptr;
    ret_code = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    EXPECT_EQ(ret_code, SQLITE_OK);

    int found = 0;
    while ((ret_code = sqlite3_step(stmt)) == SQLITE_ROW) 
    {
        ++found;
    }

    EXPECT_EQ(found, 2);

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    // cleanup
    if (fs::exists(tmp))
    {
        fs::remove(tmp);
    } 
}

TEST_F(StorageManagerTest, SaveAndGetFamilyData) 
{
    Family family("Doe Family");
    family.addMember(Member("John Doe", "JD"));
    family.addMember(Member("Jane Doe", "Jane"));
    
    EXPECT_TRUE(sm->saveFamilyData(family));
    EXPECT_EQ(getTableRowCount("FamilyInfo"), 1);
    EXPECT_EQ(getTableRowCount("MemberInfo"), 2);
    
    // Get family by ID 1 (first inserted row)
    std::unique_ptr<Family> retrieved(sm->getFamilyData(1));
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->getName(), "Doe Family");
    EXPECT_EQ(retrieved->getMembers().size(), 2);
}

TEST_F(StorageManagerTest, DeleteMemberData) 
{
    // First create a family
    Family family("Doe Family");
    EXPECT_TRUE(sm->saveFamilyData(family));
    int64_t familyId = getLastInsertId("FamilyInfo");
    
    // Now create two members in that family
    sqlite3* db = nullptr;
    int ret_code = sqlite3_open(tmp_path.string().c_str(), &db);
    ASSERT_EQ(ret_code, SQLITE_OK);
    
    const char* sql = "INSERT INTO MemberInfo (Family_ID, Member_Name, Member_Nick_Name) VALUES (?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    ret_code = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    ASSERT_EQ(ret_code, SQLITE_OK);
    
    // Insert first member
    sqlite3_bind_int64(stmt, 1, familyId);
    sqlite3_bind_text(stmt, 2, "John Doe", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, "JD", -1, SQLITE_TRANSIENT);
    ret_code = sqlite3_step(stmt);
    ASSERT_EQ(ret_code, SQLITE_DONE);
    sqlite3_reset(stmt);
    
    // Insert second member
    sqlite3_bind_int64(stmt, 1, familyId);
    sqlite3_bind_text(stmt, 2, "Jane Doe", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, "Jane", -1, SQLITE_TRANSIENT);
    ret_code = sqlite3_step(stmt);
    ASSERT_EQ(ret_code, SQLITE_DONE);
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    
    EXPECT_EQ(getTableRowCount("MemberInfo"), 2);
    
    // Delete first member
    EXPECT_TRUE(sm->deleteMemberData(1));
    EXPECT_EQ(getTableRowCount("MemberInfo"), 1);
    
    // Verify first member is gone but second exists
    EXPECT_EQ(sm->getMemberData(1), nullptr);
    std::unique_ptr<Member> member2_data(sm->getMemberData(2));
    ASSERT_NE(member2_data, nullptr);
    EXPECT_EQ(member2_data->getName(), "Jane Doe");
}

TEST_F(StorageManagerTest, DeleteFamilyCascadeDeletesMembers) 
{
    // Create a family with members
    Family family("Doe Family");
    family.addMember(Member("John Doe", "JD"));
    family.addMember(Member("Jane Doe", "Jane"));
    EXPECT_TRUE(sm->saveFamilyData(family));
    EXPECT_EQ(getTableRowCount("FamilyInfo"), 1);
    EXPECT_EQ(getTableRowCount("MemberInfo"), 2);
    
    // Delete family
    EXPECT_TRUE(sm->deleteFamilyData(1));
    
    // Verify both family and members are gone (cascade delete)
    EXPECT_EQ(getTableRowCount("FamilyInfo"), 0);
    EXPECT_EQ(getTableRowCount("MemberInfo"), 0);
}

TEST_F(StorageManagerTest, InvalidOperations) 
{
    // Try to get non-existent records
    EXPECT_EQ(sm->getMemberData(999), nullptr);
    EXPECT_EQ(sm->getFamilyData(999), nullptr);
    
    // Try to delete non-existent records
    EXPECT_FALSE(sm->deleteMemberData(999));
    EXPECT_FALSE(sm->deleteFamilyData(999));
}

TEST_F(StorageManagerTest, MultipleInsertAndRetrieve) 
{
    // Create and save multiple families
    Family family1("Doe Family");
    family1.addMember(Member("John Doe", "JD"));
    Family family2("Smith Family");
    family2.addMember(Member("Jane Smith", "JS"));
    
    EXPECT_TRUE(sm->saveFamilyData(family1));
    EXPECT_TRUE(sm->saveFamilyData(family2));
    
    EXPECT_EQ(getTableRowCount("FamilyInfo"), 2);
    EXPECT_EQ(getTableRowCount("MemberInfo"), 2);
    
    // Verify both families can be retrieved
    std::unique_ptr<Family> retrieved1(sm->getFamilyData(1));
    std::unique_ptr<Family> retrieved2(sm->getFamilyData(2));
    
    ASSERT_NE(retrieved1, nullptr);
    ASSERT_NE(retrieved2, nullptr);
    EXPECT_EQ(retrieved1->getName(), "Doe Family");
    EXPECT_EQ(retrieved2->getName(), "Smith Family");
}

TEST_F(StorageManagerTest, PersistenceAcrossRestarts) 
{
    // Save a family and members using the existing StorageManager (sm)
    Family family("PersistentFamily");
    family.addMember(Member("Alice", "A"));
    family.addMember(Member("Bob", "B"));

    EXPECT_TRUE(sm->saveFamilyData(family));
    EXPECT_EQ(getTableRowCount("FamilyInfo"), 1);
    EXPECT_EQ(getTableRowCount("MemberInfo"), 2);

    // Simulate application shutdown by destroying the StorageManager
    sm.reset();

    // Simulate application restart by creating a fresh StorageManager
    auto sm2 = std::make_unique<StorageManager>();
    ASSERT_TRUE(sm2->initializeDatabase(tmp_path.string()));

    // Retrieve the family inserted earlier and verify its contents
    std::unique_ptr<Family> retrieved(sm2->getFamilyData(1));
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->getName(), "PersistentFamily");
    EXPECT_EQ(retrieved->getMembers().size(), 2);
}

int main(int argc, char **argv) 
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
