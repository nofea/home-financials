#include <gtest/gtest.h>
#include "home_manager.hpp"
#include "family.hpp"
#include "member.hpp"
#include <filesystem>
#include <memory>
#include "test_helpers.hpp"

/**
 * Test fixture for HomeManager tests.
 * 
 * Sets up a clean environment for each test with a temporary database.
 */
class HomeManagerTest : public TestDbFixture
{
protected:
    void SetUp() override
    {
        initHome("home_manager");
    }

    void TearDown() override
    {
        cleanup();
    }

    // Use TestDbFixture::home() and TestDbFixture::tmp_path
};

TEST_F(HomeManagerTest, AddAndGetFamily)
{
    // Add a new family
    Family family("Test Family");
    EXPECT_EQ(home()->addFamily(family), commons::Result::Ok);

    // Retrieve the family (should be ID 1 as it's the first)
    auto retrieved = home()->getFamily(1);
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->getName(), "Test Family");

    // Try to get non-existent family
    auto not_found = home()->getFamily(999);
    EXPECT_EQ(not_found, nullptr);

    // Try to add invalid family (empty name)
    Family invalid("");
    EXPECT_EQ(home()->addFamily(invalid), commons::Result::InvalidInput);
}

TEST_F(HomeManagerTest, UpdateFamily)
{
    // First add a family
    Family family("Original Name");
    ASSERT_EQ(home()->addFamily(family), commons::Result::Ok);

    // Update with valid name
    EXPECT_EQ(home()->updateFamilyName(1, "Updated Name"), commons::Result::Ok);

    // Verify update
    auto updated = home()->getFamily(1);
    ASSERT_NE(updated, nullptr);
    EXPECT_EQ(updated->getName(), "Updated Name");

    // Try invalid updates
    EXPECT_EQ(home()->updateFamilyName(1, ""), commons::Result::InvalidInput);
    EXPECT_EQ(home()->updateFamilyName(999, "New Name"), commons::Result::NotFound);
}

TEST_F(HomeManagerTest, DeleteFamily)
{
    // Add a family with members
    Family family("Test Family");
    ASSERT_EQ(home()->addFamily(family), commons::Result::Ok);

    // Add members to the family
    Member member1("John", "JD");
    Member member2("Jane", "JN");
    ASSERT_EQ(home()->addMemberToFamily(member1, 1), commons::Result::Ok);
    ASSERT_EQ(home()->addMemberToFamily(member2, 1), commons::Result::Ok);

    // Delete the family
    EXPECT_EQ(home()->deleteFamily(1), commons::Result::Ok);

    // Verify family and members are gone
    EXPECT_EQ(home()->getFamily(1), nullptr);
    EXPECT_EQ(home()->getMember(1), nullptr);
    EXPECT_EQ(home()->getMember(2), nullptr);

    // Try to delete non-existent family
    EXPECT_EQ(home()->deleteFamily(999), commons::Result::NotFound);
}

TEST_F(HomeManagerTest, AddAndGetMember)
{
    // First create a family
    Family family("Test Family");
    ASSERT_EQ(home()->addFamily(family), commons::Result::Ok);

    // Add member to family
    Member member("John Doe", "JD");
    EXPECT_EQ(home()->addMemberToFamily(member, 1), commons::Result::Ok);

    // Retrieve the member
    auto retrieved = home()->getMember(1);
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->getName(), "John Doe");
    EXPECT_EQ(retrieved->getNickname(), "JD");

    // Try to add member to non-existent family
    EXPECT_EQ(home()->addMemberToFamily(member, 999), commons::Result::NotFound);

    // Try to add invalid member
    Member invalid("", "");
    EXPECT_EQ(home()->addMemberToFamily(invalid, 1), commons::Result::InvalidInput);
}

TEST_F(HomeManagerTest, AddMember_MaxMembersLimit)
{
    // First create a family
    Family family("LimitTestFamily");
    ASSERT_EQ(home()->addFamily(family), commons::Result::Ok);

    // Add 255 members (should succeed)
    for (int i = 0; i < 255; ++i)
    {
        Member m(std::string("Member") + std::to_string(i), "");
        auto res = home()->addMemberToFamily(m, 1);
        ASSERT_EQ(res, commons::Result::Ok) << "Failed at iteration " << i;
    }

    // Adding the 256th member should fail with MaxMembersExceeded
    Member extra("MemberExtra", "");
    EXPECT_EQ(home()->addMemberToFamily(extra, 1), commons::Result::MaxMembersExceeded);
}

TEST_F(HomeManagerTest, UpdateMember)
{
    // Setup: add family and member
    Family family("Test Family");
    ASSERT_EQ(home()->addFamily(family), commons::Result::Ok);

    Member member("Original Name", "ON");
    ASSERT_EQ(home()->addMemberToFamily(member, 1), commons::Result::Ok);

    // Update member with valid data
    EXPECT_EQ(home()->updateMember(1, "Updated Name", "UN"), commons::Result::Ok);

    // Verify update
    auto updated = home()->getMember(1);
    ASSERT_NE(updated, nullptr);
    EXPECT_EQ(updated->getName(), "Updated Name");
    EXPECT_EQ(updated->getNickname(), "UN");

    // Test partial updates
    EXPECT_EQ(home()->updateMember(1, "", "UpdatedNick"), commons::Result::Ok); // Only nickname update
    auto nickUpdated = home()->getMember(1);
    ASSERT_NE(nickUpdated, nullptr);
    EXPECT_EQ(nickUpdated->getName(), "Updated Name"); // Name should remain unchanged
    EXPECT_EQ(nickUpdated->getNickname(), "UpdatedNick");

    EXPECT_EQ(home()->updateMember(1, "FinalName", ""), commons::Result::Ok); // Only name update
    auto nameUpdated = home()->getMember(1);
    ASSERT_NE(nameUpdated, nullptr);
    EXPECT_EQ(nameUpdated->getName(), "FinalName");
    EXPECT_EQ(nameUpdated->getNickname(), "UpdatedNick"); // Nickname should remain unchanged

    // Test invalid updates - member not found
    EXPECT_EQ(home()->updateMember(999, "Name", "Nick"), commons::Result::NotFound);

    // Test invalid updates - no fields to update
    EXPECT_EQ(home()->updateMember(1, "", ""), commons::Result::InvalidInput);
}

TEST_F(HomeManagerTest, DeleteMember)
{
    // Setup: add family and members
    Family family("Test Family");
    ASSERT_EQ(home()->addFamily(family), commons::Result::Ok);

    Member member1("John", "J");
    Member member2("Jane", "JN");
    ASSERT_EQ(home()->addMemberToFamily(member1, 1), commons::Result::Ok);
    ASSERT_EQ(home()->addMemberToFamily(member2, 1), commons::Result::Ok);

    // Delete first member
    EXPECT_EQ(home()->deleteMember(1), commons::Result::Ok);

    // Verify first member gone but second exists
    EXPECT_EQ(home()->getMember(1), nullptr);
    auto remaining = home()->getMember(2);
    ASSERT_NE(remaining, nullptr);
    EXPECT_EQ(remaining->getName(), "Jane");

    // Try to delete non-existent member
    EXPECT_EQ(home()->deleteMember(999), commons::Result::NotFound);
}

// Net worth related tests consolidated here
TEST_F(HomeManagerTest, MemberNetWorthSum)
{
    // Create family and add member via HomeManager
    Family family("NetFamilyHM");
    EXPECT_EQ(home()->addFamily(family), commons::Result::Ok);
    uint64_t family_id = home()->listFamilies().front().getId();

    Member member("Alice", "A");
    EXPECT_EQ(home()->addMemberToFamily(member, family_id), commons::Result::Ok);
    uint64_t member_id = home()->listMembersOfFamily(family_id).front().getId();

    // Resolve a bank id and add accounts using StorageManager
    auto* storage = home()->getStorageManager();
    uint64_t bank_id = 0;
    EXPECT_EQ(storage->getBankIdByName("Canara", &bank_id), commons::Result::Ok);

    // Insert two bank accounts for the member with known closing balances
    EXPECT_EQ(storage->saveBankAccountEx(bank_id, member_id, "ACC1", 10000, 15000, nullptr), commons::Result::Ok);
    EXPECT_EQ(storage->saveBankAccountEx(bank_id, member_id, "ACC2", 5000, 25000, nullptr), commons::Result::Ok);

    long long net_paise = 0;
    EXPECT_EQ(home()->computeMemberNetWorth(member_id, &net_paise), commons::Result::Ok);
    EXPECT_EQ(net_paise, 15000 + 25000);
}

TEST_F(HomeManagerTest, FamilyNetWorthSum)
{
    // Create family
    Family family("FamilyTotalHM");
    EXPECT_EQ(home()->addFamily(family), commons::Result::Ok);
    uint64_t family_id = home()->listFamilies().front().getId();

    // Add two members
    Member m1("Bob", "B");
    Member m2("Carol", "C");
    EXPECT_EQ(home()->addMemberToFamily(m1, family_id), commons::Result::Ok);
    EXPECT_EQ(home()->addMemberToFamily(m2, family_id), commons::Result::Ok);

    auto members = home()->listMembersOfFamily(family_id);
    ASSERT_EQ(members.size(), 2);
    uint64_t id1 = members[0].getId();
    uint64_t id2 = members[1].getId();

    auto* storage = home()->getStorageManager();
    uint64_t bank_id = 0;
    EXPECT_EQ(storage->getBankIdByName("SBI", &bank_id), commons::Result::Ok);

    // Add accounts: member1 -> 1000 paise, member2 -> 2000 + 3000 paise
    EXPECT_EQ(storage->saveBankAccountEx(bank_id, id1, "BACC", 0, 1000, nullptr), commons::Result::Ok);
    EXPECT_EQ(storage->saveBankAccountEx(bank_id, id2, "CACC1", 0, 2000, nullptr), commons::Result::Ok);
    EXPECT_EQ(storage->saveBankAccountEx(bank_id, id2, "CACC2", 0, 3000, nullptr), commons::Result::Ok);

    long long family_paise = 0;
    EXPECT_EQ(home()->computeFamilyNetWorth(family_id, &family_paise), commons::Result::Ok);
    EXPECT_EQ(family_paise, 1000 + 2000 + 3000);
}

TEST_F(HomeManagerTest, MemberNotFoundNetWorth)
{
    long long out = 0;
    EXPECT_EQ(home()->computeMemberNetWorth(9999, &out), commons::Result::NotFound);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}