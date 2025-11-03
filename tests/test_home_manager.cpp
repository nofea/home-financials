#include <gtest/gtest.h>
#include "home_manager.hpp"
#include "family.hpp"
#include "member.hpp"
#include <filesystem>
#include <memory>

/**
 * Test fixture for HomeManager tests.
 * 
 * Sets up a clean environment for each test with a temporary database.
 */
class HomeManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Clean up any existing test DB
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

    std::filesystem::path tmp_path{std::filesystem::temp_directory_path() / "homefinancials_test.db"};
    std::unique_ptr<HomeManager> hm;
};

TEST_F(HomeManagerTest, AddAndGetFamily)
{
    // Add a new family
    Family family("Test Family");
    EXPECT_EQ(hm->addFamily(family), StorageManager::Result::Ok);

    // Retrieve the family (should be ID 1 as it's the first)
    auto retrieved = hm->getFamily(1);
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->getName(), "Test Family");

    // Try to get non-existent family
    auto not_found = hm->getFamily(999);
    EXPECT_EQ(not_found, nullptr);

    // Try to add invalid family (empty name)
    Family invalid("");
    EXPECT_EQ(hm->addFamily(invalid), StorageManager::Result::InvalidInput);
}

TEST_F(HomeManagerTest, UpdateFamily)
{
    // First add a family
    Family family("Original Name");
    ASSERT_EQ(hm->addFamily(family), StorageManager::Result::Ok);

    // Update with valid name
    EXPECT_EQ(hm->updateFamilyName(1, "Updated Name"), StorageManager::Result::Ok);

    // Verify update
    auto updated = hm->getFamily(1);
    ASSERT_NE(updated, nullptr);
    EXPECT_EQ(updated->getName(), "Updated Name");

    // Try invalid updates
    EXPECT_EQ(hm->updateFamilyName(1, ""), StorageManager::Result::InvalidInput);
    EXPECT_EQ(hm->updateFamilyName(999, "New Name"), StorageManager::Result::NotFound);
}

TEST_F(HomeManagerTest, DeleteFamily)
{
    // Add a family with members
    Family family("Test Family");
    ASSERT_EQ(hm->addFamily(family), StorageManager::Result::Ok);

    // Add members to the family
    Member member1("John", "JD");
    Member member2("Jane", "JN");
    ASSERT_EQ(hm->addMemberToFamily(member1, 1), StorageManager::Result::Ok);
    ASSERT_EQ(hm->addMemberToFamily(member2, 1), StorageManager::Result::Ok);

    // Delete the family
    EXPECT_EQ(hm->deleteFamily(1), StorageManager::Result::Ok);

    // Verify family and members are gone
    EXPECT_EQ(hm->getFamily(1), nullptr);
    EXPECT_EQ(hm->getMember(1), nullptr);
    EXPECT_EQ(hm->getMember(2), nullptr);

    // Try to delete non-existent family
    EXPECT_EQ(hm->deleteFamily(999), StorageManager::Result::NotFound);
}

TEST_F(HomeManagerTest, AddAndGetMember)
{
    // First create a family
    Family family("Test Family");
    ASSERT_EQ(hm->addFamily(family), StorageManager::Result::Ok);

    // Add member to family
    Member member("John Doe", "JD");
    EXPECT_EQ(hm->addMemberToFamily(member, 1), StorageManager::Result::Ok);

    // Retrieve the member
    auto retrieved = hm->getMember(1);
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->getName(), "John Doe");
    EXPECT_EQ(retrieved->getNickname(), "JD");

    // Try to add member to non-existent family
    EXPECT_EQ(hm->addMemberToFamily(member, 999), StorageManager::Result::NotFound);

    // Try to add invalid member
    Member invalid("", "");
    EXPECT_EQ(hm->addMemberToFamily(invalid, 1), StorageManager::Result::InvalidInput);
}

TEST_F(HomeManagerTest, UpdateMember)
{
    // Setup: add family and member
    Family family("Test Family");
    ASSERT_EQ(hm->addFamily(family), StorageManager::Result::Ok);

    Member member("Original Name", "ON");
    ASSERT_EQ(hm->addMemberToFamily(member, 1), StorageManager::Result::Ok);

    // Update member with valid data
    EXPECT_EQ(hm->updateMember(1, "Updated Name", "UN"), StorageManager::Result::Ok);

    // Verify update
    auto updated = hm->getMember(1);
    ASSERT_NE(updated, nullptr);
    EXPECT_EQ(updated->getName(), "Updated Name");
    EXPECT_EQ(updated->getNickname(), "UN");

    // Try invalid updates
    EXPECT_EQ(hm->updateMember(1, "", "Nick"), StorageManager::Result::InvalidInput);
    EXPECT_EQ(hm->updateMember(999, "Name", "Nick"), StorageManager::Result::NotFound);
}

TEST_F(HomeManagerTest, DeleteMember)
{
    // Setup: add family and members
    Family family("Test Family");
    ASSERT_EQ(hm->addFamily(family), StorageManager::Result::Ok);

    Member member1("John", "J");
    Member member2("Jane", "JN");
    ASSERT_EQ(hm->addMemberToFamily(member1, 1), StorageManager::Result::Ok);
    ASSERT_EQ(hm->addMemberToFamily(member2, 1), StorageManager::Result::Ok);

    // Delete first member
    EXPECT_EQ(hm->deleteMember(1), StorageManager::Result::Ok);

    // Verify first member gone but second exists
    EXPECT_EQ(hm->getMember(1), nullptr);
    auto remaining = hm->getMember(2);
    ASSERT_NE(remaining, nullptr);
    EXPECT_EQ(remaining->getName(), "Jane");

    // Try to delete non-existent member
    EXPECT_EQ(hm->deleteMember(999), StorageManager::Result::NotFound);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}