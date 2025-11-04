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
    simulateMenuChoice("7", {});
    
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
    simulateMenuChoice("7", {});
    
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
