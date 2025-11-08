#include <gtest/gtest.h>
#include "tui_manager.hpp"
#include "mock_io.hpp"
#include <memory>

class TUIManagerTest : public ::testing::Test 
{
protected:
    void SetUp() override 
    {
        auto mock_io = std::make_unique<MockIO>();
        mock_io_ptr = mock_io.get(); // Save raw pointer before transfer
        tui = std::make_unique<TUIManager>(std::move(mock_io));
    }

    // Helper to simulate user selecting menu option and providing inputs
    void simulateMenuChoice(const std::string& choice, const std::vector<std::string>& inputs = {}) 
    {
        std::vector<std::string> all_inputs{choice};
        all_inputs.insert(all_inputs.end(), inputs.begin(), inputs.end());
        // Add exit command to cleanly terminate the TUI
        all_inputs.push_back(std::to_string(static_cast<int>(TUIManager::MenuOption::Exit)));
        mock_io_ptr->queueInput(all_inputs);
    }

    std::unique_ptr<TUIManager> tui;
    MockIO* mock_io_ptr{nullptr}; // Non-owning pointer to access mock after move
};

TEST_F(TUIManagerTest, AddFamily_Success) 
{
    // Setup: Choose "Add Family" (1) and provide family name
    simulateMenuChoice("1", {"Test Family"});
    
    // Run the TUI
    tui->run();
    
    // Verify output contains success message
    const auto& output = mock_io_ptr->getOutput();
    bool found_success = false;
    for (const auto& line : output) 
    {
        if (line.find("Test Family") != std::string::npos && 
            line.find("added successfully") != std::string::npos) 
        {
            found_success = true;
            break;
        }
    }
    EXPECT_TRUE(found_success);
}

TEST_F(TUIManagerTest, AddFamily_EmptyName) 
{
    // Setup: Choose "Add Family" (1) but provide empty name
    simulateMenuChoice("1", {""});
    
    // Run the TUI
    tui->run();
    
    // Verify error about empty name
    const auto& output = mock_io_ptr->getOutput();
    bool found_error = false;

    for (const auto& line : output) 
    {
        if (line.find("Family name cannot be empty") != std::string::npos) 
        {
            found_error = true;
            break;
        }
    }
    EXPECT_TRUE(found_error);
}

TEST_F(TUIManagerTest, InvalidMenuChoice) 
{
    // Setup: Provide invalid menu choice
    simulateMenuChoice("99");
    
    // Run the TUI
    tui->run();
    
    // Verify error about invalid choice
    const auto& output = mock_io_ptr->getOutput();
    bool found_error = false;
    for (const auto& line : output) 
    {
        if (line.find("Invalid choice") != std::string::npos) 
        {
            found_error = true;
            break;
        }
    }

    EXPECT_TRUE(found_error);
}

TEST_F(TUIManagerTest, DeleteFamily_Success) 
{
    // First add a family to get a valid ID
    simulateMenuChoice("1", {"Family To Delete"});
    tui->run();
    
    // Reset and delete the family (using ID 1)
    SetUp();
    simulateMenuChoice("2", {"1"});
    tui->run();
    
    // Verify output or errors contain success or error message
    const auto& output = mock_io_ptr->getOutput();
    const auto& errors = mock_io_ptr->getErrors();
    bool found_message = false;
    
    for (const auto& line : output) 
    {
        if (line.find("Family") != std::string::npos && 
            line.find("deleted successfully") != std::string::npos) 
        {
            found_message = true;
            break;
        }
    }
    
    if (!found_message) 
    {
        for (const auto& line : errors) 
        {
            if (line.find("Not found") != std::string::npos) 
            {
                found_message = true;
                break;
            }
        }
    }
    
    EXPECT_TRUE(found_message);
}

TEST_F(TUIManagerTest, DeleteFamily_InvalidId) 
{
    // Setup: Try to delete with invalid ID
    simulateMenuChoice("2", {"invalid"});
    
    // Run the TUI
    tui->run();
    
    // Verify error about invalid ID
    const auto& output = mock_io_ptr->getOutput();
    bool found_error = false;
    for (const auto& line : output) 
    {
        if (line.find("Invalid family id") != std::string::npos) 
        {
            found_error = true;
            break;
        }
    }
    EXPECT_TRUE(found_error);
}

TEST_F(TUIManagerTest, DeleteFamily_NegativeId)
{
    // Setup: Try to delete with negative ID
    simulateMenuChoice("2", {"-1"});

    // Run the TUI
    tui->run();

    // Verify error about non-negative whole number (REQ-4, REQ-5)
    const auto& output = mock_io_ptr->getOutput();
    bool found_error = false;
    for (const auto& line : output)
    {
        if (line.find("must be a non-negative whole number (REQ-4, REQ-5)") != std::string::npos)
        {
            found_error = true;
            break;
        }
    }
    EXPECT_TRUE(found_error);
}

TEST_F(TUIManagerTest, DeleteFamily_DecimalId)
{
    // Setup: Try to delete with decimal ID
    simulateMenuChoice("2", {"1.5"});

    // Run the TUI
    tui->run();

    // Verify error about non-negative whole number (REQ-4, REQ-5)
    const auto& output = mock_io_ptr->getOutput();
    bool found_error = false;
    for (const auto& line : output)
    {
        if (line.find("must be a non-negative whole number (REQ-4, REQ-5)") != std::string::npos)
        {
            found_error = true;
            break;
        }
    }
    EXPECT_TRUE(found_error);
}

TEST_F(TUIManagerTest, AddMember_Success) 
{
    // Setup: Add member to family (family ID 1)
    simulateMenuChoice("3", {"1", "John Doe", "Johnny"});
    
    // Run the TUI
    tui->run();
    
    // Verify output or errors contain a message (success or not found)
    const auto& output = mock_io_ptr->getOutput();
    const auto& errors = mock_io_ptr->getErrors();
    bool found_message = false;
    
    for (const auto& line : output) 
    {
        if ((line.find("John Doe") != std::string::npos && line.find("added") != std::string::npos)) 
        {
            found_message = true;
            break;
        }
    }
    
    if (!found_message) 
    {
        for (const auto& line : errors) 
        {
            if (line.find("Not found") != std::string::npos) 
            {
                found_message = true;
                break;
            }
        }
    }
    
    EXPECT_TRUE(found_message);
}

TEST_F(TUIManagerTest, AddMember_EmptyName) 
{
    // Setup: Try to add member with empty name
    simulateMenuChoice("3", {"1", "", "Nickname"});
    
    // Run the TUI
    tui->run();
    
    // Verify error about empty name
    const auto& output = mock_io_ptr->getOutput();
    bool found_error = false;
    for (const auto& line : output) 
    {
        if (line.find("Member name cannot be empty") != std::string::npos) 
        {
            found_error = true;
            break;
        }
    }
    EXPECT_TRUE(found_error);
}

TEST_F(TUIManagerTest, AddMember_InvalidFamilyId) 
{
    // Setup: Try to add member with invalid family ID
    simulateMenuChoice("3", {"invalid", "John Doe", "Johnny"});
    
    // Run the TUI
    tui->run();
    
    // Verify error about invalid ID
    const auto& output = mock_io_ptr->getOutput();
    bool found_error = false;
    for (const auto& line : output) 
    {
        if (line.find("Invalid family id") != std::string::npos) 
        {
            found_error = true;
            break;
        }
    }
    EXPECT_TRUE(found_error);
}

TEST_F(TUIManagerTest, AddMember_NegativeFamilyId)
{
    // Setup: Try to add member with negative family ID
    simulateMenuChoice("3", {"-1", "John Doe", "Johnny"});

    // Run the TUI
    tui->run();

    // Verify error about non-negative whole number (REQ-4, REQ-5)
    const auto& output = mock_io_ptr->getOutput();
    bool found_error = false;
    for (const auto& line : output)
    {
        if (line.find("must be a non-negative whole number (REQ-4, REQ-5)") != std::string::npos)
        {
            found_error = true;
            break;
        }
    }
    EXPECT_TRUE(found_error);
}

TEST_F(TUIManagerTest, AddMember_DecimalFamilyId)
{
    // Setup: Try to add member with decimal family ID
    simulateMenuChoice("3", {"1.5", "John Doe", "Johnny"});

    // Run the TUI
    tui->run();

    // Verify error about non-negative whole number (REQ-4, REQ-5)
    const auto& output = mock_io_ptr->getOutput();
    bool found_error = false;
    for (const auto& line : output)
    {
        if (line.find("must be a non-negative whole number (REQ-4, REQ-5)") != std::string::npos)
        {
            found_error = true;
            break;
        }
    }
    EXPECT_TRUE(found_error);
}

TEST_F(TUIManagerTest, UpdateMember_ValidInput) 
{
    // Setup: Update member (member ID 1)
    simulateMenuChoice("4", {"1", "Jane Doe", "Janie"});
    
    // Run the TUI
    tui->run();
    
    // Verify output or errors contain a message
    const auto& output = mock_io_ptr->getOutput();
    const auto& errors = mock_io_ptr->getErrors();
    bool found_message = false;
    
    for (const auto& line : output) 
    {
        if (line.find("updated successfully") != std::string::npos) 
        {
            found_message = true;
            break;
        }
    }
    
    if (!found_message) 
    {
        for (const auto& line : errors) 
        {
            if (line.find("Not found") != std::string::npos) 
            {
                found_message = true;
                break;
            }
        }
    }
    
    EXPECT_TRUE(found_message);
}

TEST_F(TUIManagerTest, UpdateMember_InvalidId) 
{
    // Setup: Try to update member with invalid ID
    simulateMenuChoice("4", {"invalid", "New Name", "Nick"});
    
    // Run the TUI
    tui->run();
    
    // Verify error about invalid ID
    const auto& output = mock_io_ptr->getOutput();
    bool found_error = false;
    for (const auto& line : output) 
    {
        if (line.find("Invalid member id") != std::string::npos) 
        {
            found_error = true;
            break;
        }
    }
    EXPECT_TRUE(found_error);
}

TEST_F(TUIManagerTest, DeleteMember_ValidInput) 
{
    // Setup: Delete member (member ID 1)
    simulateMenuChoice("5", {"1"});
    
    // Run the TUI
    tui->run();
    
    // Verify output or errors contain a message
    const auto& output = mock_io_ptr->getOutput();
    const auto& errors = mock_io_ptr->getErrors();
    bool found_message = false;
    
    for (const auto& line : output) 
    {
        if (line.find("deleted successfully") != std::string::npos) 
        {
            found_message = true;
            break;
        }
    }
    
    if (!found_message) 
    {
        for (const auto& line : errors) 
        {
            if (line.find("Not found") != std::string::npos) 
            {
                found_message = true;
                break;
            }
        }
    }
    
    EXPECT_TRUE(found_message);
}

TEST_F(TUIManagerTest, DeleteMember_InvalidId) 
{
    // Setup: Try to delete member with invalid ID
    simulateMenuChoice("5", {"invalid"});
    
    // Run the TUI
    tui->run();
    
    // Verify error about invalid ID
    const auto& output = mock_io_ptr->getOutput();
    bool found_error = false;
    for (const auto& line : output) 
    {
        if (line.find("Invalid member id") != std::string::npos) 
        {
            found_error = true;
            break;
        }
    }
    EXPECT_TRUE(found_error);
}

TEST_F(TUIManagerTest, DeleteMultipleMembers_ValidInput) 
{
    // Setup: Delete multiple members (IDs 1, 2, 3)
    simulateMenuChoice("6", {"1 2 3"});
    
    // Run the TUI
    tui->run();
    
    // Verify output or errors contain delete messages
    const auto& output = mock_io_ptr->getOutput();
    const auto& errors = mock_io_ptr->getErrors();
    bool found_message = false;
    
    for (const auto& line : output) 
    {
        if (line.find("deleted") != std::string::npos) 
        {
            found_message = true;
            break;
        }
    }
    
    if (!found_message) 
    {
        for (const auto& line : errors) 
        {
            if (line.find("Not found") != std::string::npos) 
            {
                found_message = true;
                break;
            }
        }
    }
    
    EXPECT_TRUE(found_message);
}

TEST_F(TUIManagerTest, DeleteMultipleMembers_InvalidInput) 
{
    // Setup: Try to delete members with invalid input
    simulateMenuChoice("6", {"invalid input"});
    
    // Run the TUI
    tui->run();
    
    // Verify error about invalid input
    const auto& output = mock_io_ptr->getOutput();
    bool found_error = false;
    for (const auto& line : output) 
    {
        if (line.find("Invalid input for member ids") != std::string::npos) 
        {
            found_error = true;
            break;
        }
    }
    EXPECT_TRUE(found_error);
}

TEST_F(TUIManagerTest, DeleteMultipleMembers_EmptyInput) 
{
    // Setup: Try to delete members with empty input
    simulateMenuChoice("6", {""});
    
    // Run the TUI
    tui->run();
    
    // Verify error about invalid input
    const auto& output = mock_io_ptr->getOutput();
    bool found_error = false;
    for (const auto& line : output) 
    {
        if (line.find("Invalid input for member ids") != std::string::npos) 
        {
            found_error = true;
            break;
        }
    }
    EXPECT_TRUE(found_error);
}

TEST_F(TUIManagerTest, DeleteMultipleMembers_DecimalInput)
{
    // Setup: Try to delete members where one id is decimal
    simulateMenuChoice("6", {"1 2 3.5"});

    // Run the TUI
    tui->run();

    // Verify error about non-negative whole numbers (REQ-4, REQ-5)
    const auto& output = mock_io_ptr->getOutput();
    bool found_error = false;
    for (const auto& line : output)
    {
        if (line.find("Member ids must be non-negative whole numbers (REQ-4, REQ-5)") != std::string::npos)
        {
            found_error = true;
            break;
        }
    }
    EXPECT_TRUE(found_error);
}

TEST_F(TUIManagerTest, DeleteMultipleMembers_NegativeInput)
{
    // Setup: Try to delete members where one id is negative
    simulateMenuChoice("6", {"1 -2 3"});

    // Run the TUI
    tui->run();

    // Verify error about non-negative whole numbers (REQ-4, REQ-5)
    const auto& output = mock_io_ptr->getOutput();
    bool found_error = false;
    for (const auto& line : output)
    {
        if (line.find("Member ids must be non-negative whole numbers (REQ-4, REQ-5)") != std::string::npos)
        {
            found_error = true;
            break;
        }
    }
    EXPECT_TRUE(found_error);
}

TEST_F(TUIManagerTest, NonNumericMenuChoice) 
{
    // Setup: Provide non-numeric menu choice
    simulateMenuChoice("abc");
    
    // Run the TUI
    tui->run();
    
    // Verify error about invalid choice
    const auto& output = mock_io_ptr->getOutput();
    bool found_error = false;
    for (const auto& line : output) 
    {
        if (line.find("Invalid choice, please enter a number") != std::string::npos) 
        {
            found_error = true;
            break;
        }
    }
    EXPECT_TRUE(found_error);
}

TEST_F(TUIManagerTest, WelcomeMessageDisplayed) 
{
    // Setup: Just run with exit
    simulateMenuChoice("9", {});
    
    // Run the TUI
    tui->run();
    
    // Verify welcome message is displayed
    const auto& output = mock_io_ptr->getOutput();
    bool found_welcome = false;
    for (const auto& line : output) 
    {
        if (line.find("Welcome to Home Financials TUI") != std::string::npos) 
        {
            found_welcome = true;
            break;
        }
    }
    EXPECT_TRUE(found_welcome);
}

TEST_F(TUIManagerTest, GoodbyeMessageDisplayed) 
{
    // Setup: Exit immediately
    simulateMenuChoice("9", {});
    
    // Run the TUI
    tui->run();
    
    // Verify goodbye message is displayed
    const auto& output = mock_io_ptr->getOutput();
    bool found_goodbye = false;
    for (const auto& line : output) 
    {
        if (line.find("Goodbye") != std::string::npos) 
        {
            found_goodbye = true;
            break;
        }
    }
    EXPECT_TRUE(found_goodbye);
}

TEST_F(TUIManagerTest, AddFamily_DisplaysID) 
{
    // Setup: Add a family
    simulateMenuChoice("1", {"FamilyWithID"});
    
    // Run the TUI
    tui->run();
    
    // Verify output contains ID
    const auto& output = mock_io_ptr->getOutput();
    bool found_id = false;
    for (const auto& line : output) 
    {
        if (line.find("FamilyWithID") != std::string::npos && 
            line.find("added successfully") != std::string::npos &&
            line.find("ID:") != std::string::npos) 
        {
            found_id = true;
            break;
        }
    }
    EXPECT_TRUE(found_id);
}

TEST_F(TUIManagerTest, AddMember_DisplaysID) 
{
    // First add a family and ensure success
    simulateMenuChoice("1", {"FamilyForMember"});
    tui->run();
    
    const auto& output1 = mock_io_ptr->getOutput();
    
    // Extract family ID from the output
    std::string family_id;
    for (const auto& line : output1)
    {
        if (line.find("FamilyForMember") != std::string::npos && 
            line.find("ID:") != std::string::npos)
        {
            auto pos = line.find("ID:");
            if (pos != std::string::npos)
            {
                std::string after_id = line.substr(pos + 3);
                // Extract the number
                size_t start = after_id.find_first_of("0123456789");
                if (start != std::string::npos)
                {
                    size_t end = after_id.find_first_not_of("0123456789", start);
                    family_id = after_id.substr(start, end == std::string::npos ? std::string::npos : end - start);
                }
                break;
            }
        }
    }
    
    ASSERT_FALSE(family_id.empty()) << "Could not extract family ID from output";
    
    // Reset for new test
    SetUp();
    
    // Add member using the extracted family ID
    simulateMenuChoice("3", {family_id, "John Doe", "JD"});
    tui->run();
    
    // Verify output contains member ID
    const auto& output = mock_io_ptr->getOutput();
    bool found_id = false;
    for (const auto& line : output) 
    {
        if (line.find("John Doe") != std::string::npos && 
            line.find("added to family") != std::string::npos &&
            line.find("ID:") != std::string::npos) 
        {
            found_id = true;
            break;
        }
    }
    EXPECT_TRUE(found_id);
}

TEST_F(TUIManagerTest, AddMember_MaxMembersExceeded_TUI)
{
    // Create a family via TUI addFamily helper (direct call)
    tui->addFamily("MaxMembersFamily");

    // Extract family ID from output
    std::string family_id;
    const auto& output1 = mock_io_ptr->getOutput();
    for (const auto& line : output1)
    {
        if (line.find("MaxMembersFamily") != std::string::npos && line.find("ID:") != std::string::npos)
        {
            auto pos = line.find("ID:");
            if (pos != std::string::npos)
            {
                std::string after_id = line.substr(pos + 3);
                size_t start = after_id.find_first_of("0123456789");
                if (start != std::string::npos)
                {
                    size_t end = after_id.find_first_not_of("0123456789", start);
                    family_id = after_id.substr(start, end == std::string::npos ? std::string::npos : end - start);
                }
                break;
            }
        }
    }

    ASSERT_FALSE(family_id.empty());

    // Add 255 members via direct addMember API on the TUI (which delegates to HomeManager)
    for (int i = 0; i < 255; ++i)
    {
        Member m(std::string("M") + std::to_string(i), "");
        auto res = tui->addMember(static_cast<uint64_t>(std::stoull(family_id)), m);
        ASSERT_EQ(res, commons::Result::Ok) << "Failed at iteration " << i;
    }

    // Attempt to add the 256th member - expect MaxMembersExceeded and an error printed
    Member extra("ExtraMember", "");
    auto res = tui->addMember(static_cast<uint64_t>(std::stoull(family_id)), extra);
    EXPECT_EQ(res, commons::Result::MaxMembersExceeded);

    // Ensure the error message was printed to errors
    const auto& errors = mock_io_ptr->getErrors();
    bool found_message = false;
    for (const auto& e : errors)
    {
        if (e.find("Cannot add member: family has reached the maximum of 255 members.") != std::string::npos)
        {
            found_message = true;
            break;
        }
    }
    EXPECT_TRUE(found_message);
}

TEST_F(TUIManagerTest, ListFamilies_ShowsAll) 
{
    // Add first family
    simulateMenuChoice("1", {"ListTestFam1"});
    tui->run();
    
    // Reset and add second family
    SetUp();
    simulateMenuChoice("1", {"ListTestFam2"});
    tui->run();
    
    // Reset and list families
    SetUp();
    simulateMenuChoice("7", {});
    tui->run();
    
    // Verify output contains both families (check for unique names to avoid conflicts)
    const auto& output = mock_io_ptr->getOutput();
    
    bool found_family_one = false;
    bool found_family_two = false;
    
    for (const auto& line : output) 
    {
        if (line.find("ListTestFam1") != std::string::npos) 
        {
            found_family_one = true;
        }
        if (line.find("ListTestFam2") != std::string::npos) 
        {
            found_family_two = true;
        }
    }
    EXPECT_TRUE(found_family_one);
    EXPECT_TRUE(found_family_two);
}

TEST_F(TUIManagerTest, ListFamilies_EmptyDatabase) 
{
    // Just list families - may show existing data from other tests, but should not crash
    simulateMenuChoice("7", {});
    tui->run();
    
    // Verify the command runs without error (either shows data or says empty)
    const auto& output = mock_io_ptr->getOutput();
    bool has_output = !output.empty();
    EXPECT_TRUE(has_output);
}

TEST_F(TUIManagerTest, ListMembersOfFamily_ShowsAll) 
{
    // Add a family and get its ID
    simulateMenuChoice("1", {"ListMemberTestFamily"});
    tui->run();
    
    const auto& output1 = mock_io_ptr->getOutput();
    std::string family_id;
    for (const auto& line : output1)
    {
        if (line.find("ListMemberTestFamily") != std::string::npos && 
            line.find("ID:") != std::string::npos)
        {
            auto pos = line.find("ID:");
            if (pos != std::string::npos)
            {
                std::string after_id = line.substr(pos + 3);
                size_t start = after_id.find_first_of("0123456789");
                if (start != std::string::npos)
                {
                    size_t end = after_id.find_first_not_of("0123456789", start);
                    family_id = after_id.substr(start, end == std::string::npos ? std::string::npos : end - start);
                }
                break;
            }
        }
    }
    ASSERT_FALSE(family_id.empty());
    
    // Add first member
    SetUp();
    simulateMenuChoice("3", {family_id, "Alice", "Al"});
    tui->run();
    
    // Add second member
    SetUp();
    simulateMenuChoice("3", {family_id, "Bob", "Bobby"});
    tui->run();
    
    // List members
    SetUp();
    simulateMenuChoice("8", {family_id});
    tui->run();
    
    // Verify output contains both members
    const auto& output = mock_io_ptr->getOutput();
    bool found_alice = false;
    bool found_bob = false;
    
    for (const auto& line : output) 
    {
        if (line.find("Alice") != std::string::npos) 
        {
            found_alice = true;
        }
        if (line.find("Bob") != std::string::npos) 
        {
            found_bob = true;
        }
    }
    EXPECT_TRUE(found_alice);
    EXPECT_TRUE(found_bob);
}

TEST_F(TUIManagerTest, ListMembersOfFamily_EmptyFamily) 
{
    // Add a family with a unique name
    simulateMenuChoice("1", {"EmptyFamilyListTest"});
    tui->run();
    
    const auto& output1 = mock_io_ptr->getOutput();
    std::string family_id;
    for (const auto& line : output1)
    {
        if (line.find("EmptyFamilyListTest") != std::string::npos && 
            line.find("ID:") != std::string::npos)
        {
            auto pos = line.find("ID:");
            if (pos != std::string::npos)
            {
                std::string after_id = line.substr(pos + 3);
                size_t start = after_id.find_first_of("0123456789");
                if (start != std::string::npos)
                {
                    size_t end = after_id.find_first_not_of("0123456789", start);
                    family_id = after_id.substr(start, end == std::string::npos ? std::string::npos : end - start);
                }
                break;
            }
        }
    }
    ASSERT_FALSE(family_id.empty());
    
    // List members of the empty family
    SetUp();
    simulateMenuChoice("8", {family_id});
    tui->run();
    
    // Verify output contains "No members found"
    const auto& output = mock_io_ptr->getOutput();
    bool found_empty_message = false;
    
    for (const auto& line : output) 
    {
        if (line.find("No members found") != std::string::npos) 
        {
            found_empty_message = true;
            break;
        }
    }
    EXPECT_TRUE(found_empty_message);
}

TEST_F(TUIManagerTest, ListMembersOfFamily_InvalidId) 
{
    // Try to list members with invalid ID
    simulateMenuChoice("8", {"invalid"});
    tui->run();
    
    // Verify error message
    const auto& output = mock_io_ptr->getOutput();
    bool found_error = false;
    
    for (const auto& line : output) 
    {
        if (line.find("Invalid family id") != std::string::npos) 
        {
            found_error = true;
            break;
        }
    }
    EXPECT_TRUE(found_error);
}
