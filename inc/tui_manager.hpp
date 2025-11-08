#pragma once

#include "ui_manager.hpp"
#include "terminal_io.hpp"
#include <memory>
#include <vector>
#include <cstdint>

class TUIManager : public UIManager
{
public:
    // Default constructor for production use - creates TerminalIO internally
    TUIManager();
    
    // Constructor for testing - accepts injected IOInterface
    explicit TUIManager(std::unique_ptr<IOInterface> io_ptr);
    
    ~TUIManager() override;

    // Menu options for the terminal UI. Use these instead of magic numbers
    // in the interactive loop.
    enum class MenuOption : int
    {
        AddFamily = 1,
        DeleteFamily = 2,
        AddMember = 3,
        UpdateMember = 4,
        DeleteMember = 5,
        DeleteMultipleMembers = 6,
        ListFamilies = 7,
        ListMembersOfFamily = 8,
        ImportBankStatement = 9,
        Exit = 10
    };

    commons::Result addFamily(const std::string& name) override;
    commons::Result deleteFamily(const uint64_t& family_id) override;

    commons::Result addMember(const uint64_t& family_id, const Member& member) override;
    commons::Result updateMember(const uint64_t& member_id,
                                const std::string& new_name,
                                const std::string& new_nickname) override;

    commons::Result deleteMember(const uint64_t& member_id) override;
    commons::Result deleteMembers(const std::vector<uint64_t>& member_ids) override;

    // Override showError to use the IOInterface
    void showError(const commons::Result& res);

    // Start the terminal UI loop; this method handles all user interaction
    // (welcome page, menu display, input parsing and dispatch).
    void run();

private:
    std::unique_ptr<IOInterface> io_ptr;
    HomeManager home_manager;
};
