#include <gtest/gtest.h>
#include "test_helpers.hpp"
#include "storage_manager.hpp"
#include "home_manager.hpp"
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
class StorageManagerTest : public TestDbFixture
{
protected:
    void SetUp() override
    {
        initStorage("storage_manager");
    }

    void TearDown() override
    {
        cleanup();
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

    // Use TestDbFixture::tmp_path and storage()/initStorage()
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
    
    EXPECT_TRUE(storage()->saveFamilyData(family));
    EXPECT_EQ(getTableRowCount("FamilyInfo"), 1);
    EXPECT_EQ(getTableRowCount("MemberInfo"), 2);
    
    // Get family by ID 1 (first inserted row)
    std::unique_ptr<Family> retrieved(storage()->getFamilyData(1));
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->getName(), "Doe Family");
    EXPECT_EQ(retrieved->getMembers().size(), 2);
}

TEST_F(StorageManagerTest, DeleteMemberData) 
{
    // First create a family
    Family family("Doe Family");
    EXPECT_TRUE(storage()->saveFamilyData(family));
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
    EXPECT_TRUE(storage()->deleteMemberData(1));
    EXPECT_EQ(getTableRowCount("MemberInfo"), 1);
    
    // Verify first member is gone but second exists
    EXPECT_EQ(storage()->getMemberData(1), nullptr);
    std::unique_ptr<Member> member2_data(storage()->getMemberData(2));
    ASSERT_NE(member2_data, nullptr);
    EXPECT_EQ(member2_data->getName(), "Jane Doe");
}

TEST_F(StorageManagerTest, DeleteFamilyCascadeDeletesMembers) 
{
    // Create a family with members
    Family family("Doe Family");
    family.addMember(Member("John Doe", "JD"));
    family.addMember(Member("Jane Doe", "Jane"));
    EXPECT_TRUE(storage()->saveFamilyData(family));
    EXPECT_EQ(getTableRowCount("FamilyInfo"), 1);
    EXPECT_EQ(getTableRowCount("MemberInfo"), 2);
    
    // Delete family
    EXPECT_TRUE(storage()->deleteFamilyData(1));
    
    // Verify both family and members are gone (cascade delete)
    EXPECT_EQ(getTableRowCount("FamilyInfo"), 0);
    EXPECT_EQ(getTableRowCount("MemberInfo"), 0);
}

TEST_F(StorageManagerTest, InvalidOperations) 
{
    // Try to get non-existent records
    EXPECT_EQ(storage()->getMemberData(999), nullptr);
    EXPECT_EQ(storage()->getFamilyData(999), nullptr);
    
    // Try to delete non-existent records
    EXPECT_FALSE(storage()->deleteMemberData(999));
    EXPECT_FALSE(storage()->deleteFamilyData(999));
}

TEST_F(StorageManagerTest, MultipleInsertAndRetrieve) 
{
    // Create and save multiple families
    Family family1("Doe Family");
    family1.addMember(Member("John Doe", "JD"));
    Family family2("Smith Family");
    family2.addMember(Member("Jane Smith", "JS"));
    
    EXPECT_TRUE(storage()->saveFamilyData(family1));
    EXPECT_TRUE(storage()->saveFamilyData(family2));
    
    EXPECT_EQ(getTableRowCount("FamilyInfo"), 2);
    EXPECT_EQ(getTableRowCount("MemberInfo"), 2);
    
    // Verify both families can be retrieved
    std::unique_ptr<Family> retrieved1(storage()->getFamilyData(1));
    std::unique_ptr<Family> retrieved2(storage()->getFamilyData(2));
    
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

    EXPECT_TRUE(storage()->saveFamilyData(family));
    EXPECT_EQ(getTableRowCount("FamilyInfo"), 1);
    EXPECT_EQ(getTableRowCount("MemberInfo"), 2);

    // Simulate application shutdown by destroying the StorageManager (keep DB file)
    destroyStorage();

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
    EXPECT_EQ(storage()->saveFamilyDataEx(invalid_family, &family_id), commons::Result::InvalidInput);

    // Test valid family save with ID return
    Family valid_family("Test Family");
    EXPECT_EQ(storage()->saveFamilyDataEx(valid_family, &family_id), commons::Result::Ok);
    EXPECT_GT(family_id, 0);

    // Test member save validation
    Member invalid_member("", "");  // Empty name
    uint64_t member_id = 0;
    EXPECT_EQ(storage()->saveMemberDataEx(invalid_member, family_id, &member_id), commons::Result::InvalidInput);

    // Test member save with non-existent family
    Member valid_member("John", "Johnny");
    EXPECT_EQ(storage()->saveMemberDataEx(valid_member, 999, &member_id), commons::Result::NotFound);

    // Test valid member save with ID return
    EXPECT_EQ(storage()->saveMemberDataEx(valid_member, family_id, &member_id), commons::Result::Ok);
    EXPECT_GT(member_id, 0);
}

TEST_F(StorageManagerTest, ExtendedAPIsUpdateOperations) 
{
    // Setup test data
    Family family("Original Family");
    uint64_t family_id = 0;
    ASSERT_EQ(storage()->saveFamilyDataEx(family, &family_id), commons::Result::Ok);

    Member member("Original Name", "Original Nick");
    uint64_t member_id = 0;
    ASSERT_EQ(storage()->saveMemberDataEx(member, family_id, &member_id), commons::Result::Ok);

    // Test family update validations
    EXPECT_EQ(storage()->updateFamilyDataEx(family_id, ""), commons::Result::InvalidInput);
    EXPECT_EQ(storage()->updateFamilyDataEx(999, "New Name"), commons::Result::NotFound);
    EXPECT_EQ(storage()->updateFamilyDataEx(family_id, "Updated Family"), commons::Result::Ok);

    // Verify family update
    std::unique_ptr<Family> updated_family(storage()->getFamilyData(family_id));
    ASSERT_NE(updated_family, nullptr);
    EXPECT_EQ(updated_family->getName(), "Updated Family");

    // Test member update validations - full update
    EXPECT_EQ(storage()->updateMemberDataEx(999, "New Name", "New Nick"), commons::Result::NotFound);
    EXPECT_EQ(storage()->updateMemberDataEx(member_id, "Updated Name", "Updated Nick"), commons::Result::Ok);

    // Verify full member update
    std::unique_ptr<Member> updated_member(storage()->getMemberData(member_id));
    ASSERT_NE(updated_member, nullptr);
    EXPECT_EQ(updated_member->getName(), "Updated Name");
    EXPECT_EQ(updated_member->getNickname(), "Updated Nick");

    // Test member partial updates
    // Update only nickname
    EXPECT_EQ(storage()->updateMemberDataEx(member_id, "", "New Nickname Only"), commons::Result::Ok);
    updated_member.reset(storage()->getMemberData(member_id));
    ASSERT_NE(updated_member, nullptr);
    EXPECT_EQ(updated_member->getName(), "Updated Name");  // Name unchanged
    EXPECT_EQ(updated_member->getNickname(), "New Nickname Only");

    // Update only name
    EXPECT_EQ(storage()->updateMemberDataEx(member_id, "New Name Only", ""), commons::Result::Ok);
    updated_member.reset(storage()->getMemberData(member_id));
    ASSERT_NE(updated_member, nullptr);
    EXPECT_EQ(updated_member->getName(), "New Name Only");
    EXPECT_EQ(updated_member->getNickname(), "New Nickname Only");  // Nickname unchanged

    // Try to update with no fields (should fail)
    EXPECT_EQ(storage()->updateMemberDataEx(member_id, "", ""), commons::Result::InvalidInput);
}

TEST_F(StorageManagerTest, ExtendedAPIsDeleteOperations) 
{
    // Setup test data
    Family family("Test Family");
    uint64_t family_id = 0;
    ASSERT_EQ(storage()->saveFamilyDataEx(family, &family_id), commons::Result::Ok);

    Member member1("Member 1", "M1");
    Member member2("Member 2", "M2");
    uint64_t member1_id = 0, member2_id = 0;
    ASSERT_EQ(storage()->saveMemberDataEx(member1, family_id, &member1_id), commons::Result::Ok);
    ASSERT_EQ(storage()->saveMemberDataEx(member2, family_id, &member2_id), commons::Result::Ok);

    // Test member deletion
    EXPECT_EQ(storage()->deleteMemberDataEx(999), commons::Result::NotFound);
    EXPECT_EQ(storage()->deleteMemberDataEx(member1_id), commons::Result::Ok);
    EXPECT_EQ(getTableRowCount("MemberInfo"), 1);  // One member remains

    // Test cascade delete via family deletion
    EXPECT_EQ(storage()->deleteFamilyDataEx(999), commons::Result::NotFound);
    EXPECT_EQ(storage()->deleteFamilyDataEx(family_id), commons::Result::Ok);
    EXPECT_EQ(getTableRowCount("FamilyInfo"), 0);
    EXPECT_EQ(getTableRowCount("MemberInfo"), 0);  // Cascade delete worked
}

// Storage-related small tests consolidated here (previously in test_storage_banklist_and_save_errors.cpp)
class StorageBankListTest : public TestDbFixture
{
protected:
    void SetUp() override
    {
        initHome("storage_banklist");
    }

    void TearDown() override
    {
        cleanup();
    }

    // Use TestDbFixture::home() and TestDbFixture::tmp_path
};

TEST_F(StorageBankListTest, BankListPrepopulatedAndCaseInsensitive)
{
    uint64_t id1 = 0;
    auto res = home()->getStorageManager()->getBankIdByName("Canara", &id1);
    EXPECT_EQ(res, commons::Result::Ok);
    EXPECT_GT(id1, 0u);

    uint64_t id2 = 0;
    auto res2 = home()->getStorageManager()->getBankIdByName("canara", &id2);
    EXPECT_EQ(res2, commons::Result::Ok);
    EXPECT_EQ(id1, id2);
}

TEST_F(StorageBankListTest, SaveBankAccountErrorsWhenMissingRefs)
{
    // Create a family and member so we can test missing bank and missing member
    Family f("F1");
    ASSERT_EQ(home()->addFamily(f), commons::Result::Ok);
    Member m("Bob", "B");
    uint64_t member_id = 0;
    ASSERT_EQ(home()->addMemberToFamily(m, 1, &member_id), commons::Result::Ok);
    EXPECT_GT(member_id, 0u);

    // Use an invalid bank id - should return NotFound
    auto s = home()->getStorageManager();
    auto res = s->saveBankAccountEx(999999, member_id, "acc", 1000ll, 2000ll, nullptr);
    EXPECT_EQ(res, commons::Result::NotFound);

    // Use an invalid member id - should return NotFound
    uint64_t bank_id = 0;
    ASSERT_EQ(s->getBankIdByName("Canara", &bank_id), commons::Result::Ok);
    auto res2 = s->saveBankAccountEx(bank_id, 999999, "acc", 1000ll, 2000ll, nullptr);
    EXPECT_EQ(res2, commons::Result::NotFound);
}

int main(int argc, char **argv) 
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
