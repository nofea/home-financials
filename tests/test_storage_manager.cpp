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

TEST_F(StorageManagerTest, ExtendedAPIsValidation) 
{
    // Test invalid family name
    Family invalid_family("");
    uint64_t family_id = 0;
    EXPECT_EQ(sm->saveFamilyDataEx(invalid_family, &family_id), StorageManager::Result::InvalidInput);

    // Test valid family save with ID return
    Family valid_family("Test Family");
    EXPECT_EQ(sm->saveFamilyDataEx(valid_family, &family_id), StorageManager::Result::Ok);
    EXPECT_GT(family_id, 0);

    // Test member save validation
    Member invalid_member("", "");  // Empty name
    uint64_t member_id = 0;
    EXPECT_EQ(sm->saveMemberDataEx(invalid_member, family_id, &member_id), StorageManager::Result::InvalidInput);

    // Test member save with non-existent family
    Member valid_member("John", "Johnny");
    EXPECT_EQ(sm->saveMemberDataEx(valid_member, 999, &member_id), StorageManager::Result::NotFound);

    // Test valid member save with ID return
    EXPECT_EQ(sm->saveMemberDataEx(valid_member, family_id, &member_id), StorageManager::Result::Ok);
    EXPECT_GT(member_id, 0);
}

TEST_F(StorageManagerTest, ExtendedAPIsUpdateOperations) 
{
    // Setup test data
    Family family("Original Family");
    uint64_t family_id = 0;
    ASSERT_EQ(sm->saveFamilyDataEx(family, &family_id), StorageManager::Result::Ok);

    Member member("Original Name", "Original Nick");
    uint64_t member_id = 0;
    ASSERT_EQ(sm->saveMemberDataEx(member, family_id, &member_id), StorageManager::Result::Ok);

    // Test family update validations
    EXPECT_EQ(sm->updateFamilyDataEx(family_id, ""), StorageManager::Result::InvalidInput);
    EXPECT_EQ(sm->updateFamilyDataEx(999, "New Name"), StorageManager::Result::NotFound);
    EXPECT_EQ(sm->updateFamilyDataEx(family_id, "Updated Family"), StorageManager::Result::Ok);

    // Verify family update
    std::unique_ptr<Family> updated_family(sm->getFamilyData(family_id));
    ASSERT_NE(updated_family, nullptr);
    EXPECT_EQ(updated_family->getName(), "Updated Family");

    // Test member update validations - full update
    EXPECT_EQ(sm->updateMemberDataEx(999, "New Name", "New Nick"), StorageManager::Result::NotFound);
    EXPECT_EQ(sm->updateMemberDataEx(member_id, "Updated Name", "Updated Nick"), StorageManager::Result::Ok);

    // Verify full member update
    std::unique_ptr<Member> updated_member(sm->getMemberData(member_id));
    ASSERT_NE(updated_member, nullptr);
    EXPECT_EQ(updated_member->getName(), "Updated Name");
    EXPECT_EQ(updated_member->getNickname(), "Updated Nick");

    // Test member partial updates
    // Update only nickname
    EXPECT_EQ(sm->updateMemberDataEx(member_id, "", "New Nickname Only"), StorageManager::Result::Ok);
    updated_member.reset(sm->getMemberData(member_id));
    ASSERT_NE(updated_member, nullptr);
    EXPECT_EQ(updated_member->getName(), "Updated Name");  // Name unchanged
    EXPECT_EQ(updated_member->getNickname(), "New Nickname Only");

    // Update only name
    EXPECT_EQ(sm->updateMemberDataEx(member_id, "New Name Only", ""), StorageManager::Result::Ok);
    updated_member.reset(sm->getMemberData(member_id));
    ASSERT_NE(updated_member, nullptr);
    EXPECT_EQ(updated_member->getName(), "New Name Only");
    EXPECT_EQ(updated_member->getNickname(), "New Nickname Only");  // Nickname unchanged

    // Try to update with no fields (should fail)
    EXPECT_EQ(sm->updateMemberDataEx(member_id, "", ""), StorageManager::Result::InvalidInput);
}

TEST_F(StorageManagerTest, ExtendedAPIsDeleteOperations) 
{
    // Setup test data
    Family family("Test Family");
    uint64_t family_id = 0;
    ASSERT_EQ(sm->saveFamilyDataEx(family, &family_id), StorageManager::Result::Ok);

    Member member1("Member 1", "M1");
    Member member2("Member 2", "M2");
    uint64_t member1_id = 0, member2_id = 0;
    ASSERT_EQ(sm->saveMemberDataEx(member1, family_id, &member1_id), StorageManager::Result::Ok);
    ASSERT_EQ(sm->saveMemberDataEx(member2, family_id, &member2_id), StorageManager::Result::Ok);

    // Test member deletion
    EXPECT_EQ(sm->deleteMemberDataEx(999), StorageManager::Result::NotFound);
    EXPECT_EQ(sm->deleteMemberDataEx(member1_id), StorageManager::Result::Ok);
    EXPECT_EQ(getTableRowCount("MemberInfo"), 1);  // One member remains

    // Test cascade delete via family deletion
    EXPECT_EQ(sm->deleteFamilyDataEx(999), StorageManager::Result::NotFound);
    EXPECT_EQ(sm->deleteFamilyDataEx(family_id), StorageManager::Result::Ok);
    EXPECT_EQ(getTableRowCount("FamilyInfo"), 0);
    EXPECT_EQ(getTableRowCount("MemberInfo"), 0);  // Cascade delete worked
}

int main(int argc, char **argv) 
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
