#pragma once

#include "ui_manager.hpp"
#include "home_manager.hpp"
#include "io_interface.hpp"
#include <memory>
#include <vector>
#include <cstdint>

class TUIManager : public UIManager
{
public:
    // Constructor taking an optional I/O interface. Defaults to TerminalIO
    // if none provided.
    explicit TUIManager(std::unique_ptr<IOInterface> io = std::make_unique<TerminalIO>());
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
        Exit = 7
    };

    commons::Result addFamily(const std::string& name) override;
    commons::Result deleteFamily(uint64_t family_id) override;

    commons::Result addMember(uint64_t family_id, const Member& member) override;
    commons::Result updateMember(uint64_t family_id,
                                uint64_t member_id,
                                const std::string& new_name,
                                const std::string& new_nickname) override;

    commons::Result deleteMember(uint64_t family_id, uint64_t member_id) override;
    commons::Result deleteMembers(uint64_t family_id, const std::vector<uint64_t>& member_ids) override;

    // Override showError to use the IOInterface
    void showError(commons::Result res) const;

    // Start the terminal UI loop; this method handles all user interaction
    // (welcome page, menu display, input parsing and dispatch).
    void run();

private:
    mutable std::unique_ptr<IOInterface> io;
    HomeManager home_manager;
};
