#include "tui_manager.hpp"
#include <string>
#include <sstream>
#include <algorithm>
#include <cctype>

TUIManager::TUIManager()
    : io(std::make_unique<TerminalIO>())
{
}

TUIManager::TUIManager(std::unique_ptr<IOInterface> io_)
    : io(std::move(io_))
{
}

TUIManager::~TUIManager()
{
}

void TUIManager::showError(commons::Result res)
{
    if (res == commons::Result::Ok) 
    {
        return; // nothing to show for success
    }

    io->printError(errorMessage(res));
}

commons::Result TUIManager::addFamily(const std::string& name)
{
    Family f(name);
    uint64_t new_id = 0;
    commons::Result res = home_manager.addFamily(f, &new_id);
    if (res != commons::Result::Ok) {
        showError(res);
    } else {
        io->printLine("Family '" + name + "' added successfully. ID: " + std::to_string(new_id));
    }
    return res;
}

commons::Result TUIManager::deleteFamily(uint64_t family_id)
{
    commons::Result res = home_manager.deleteFamily(family_id);
    if (res != commons::Result::Ok) {
        showError(res);
    } else {
        io->printLine("Family " + std::to_string(family_id) + " deleted successfully.");
    }
    return res;
}

commons::Result TUIManager::addMember(uint64_t family_id, const Member& member)
{
    uint64_t new_id = 0;
    commons::Result res = home_manager.addMemberToFamily(member, family_id, &new_id);
    if (res != commons::Result::Ok) {
        showError(res);
    } else {
        io->printLine("Member '" + member.getName() + "' added to family " + std::to_string(family_id) + ". ID: " + std::to_string(new_id));
    }
    return res;
}

commons::Result TUIManager::updateMember(uint64_t member_id, const std::string& new_name, const std::string& new_nickname)
{
    commons::Result res = home_manager.updateMember(member_id, new_name, new_nickname);
    if (res != commons::Result::Ok) {
        showError(res);
    } else {
        io->printLine("Member " + std::to_string(member_id) + " updated successfully.");
    }
    return res;
}

commons::Result TUIManager::deleteMember(uint64_t member_id)
{
    commons::Result res = home_manager.deleteMember(member_id);
    if (res != commons::Result::Ok) {
        showError(res);
    } else {
        io->printLine("Member " + std::to_string(member_id) + " deleted successfully.");
    }
    return res;
}

commons::Result TUIManager::deleteMembers(const std::vector<uint64_t>& member_ids)
{
    commons::Result final_res = commons::Result::Ok;
    for (const auto& id : member_ids) 
    {
        commons::Result res = home_manager.deleteMember(id);
        if (res != commons::Result::Ok) 
        {
            // print per-member error but continue attempting to delete others
            showError(res);
            if (final_res == commons::Result::Ok) 
            {
                final_res = res;
            }
        } 
        else 
        {
            io->printLine("Member " + std::to_string(id) + " deleted.");
        }
    }

    return final_res;
}

void TUIManager::run()
{
    // Welcome page
    io->printLine("Welcome to Home Financials TUI");
    io->printLine("============================");

    bool running = true;
    while (running) 
    {
        // Display menu
        io->printLine("");
        io->printLine("Select an option:");
        io->printLine(" 1) Add Family");
        io->printLine(" 2) Delete Family");
        io->printLine(" 3) Add Member to Family");
        io->printLine(" 4) Update Member");
        io->printLine(" 5) Delete Member");
        io->printLine(" 6) Delete Multiple Members");
        io->printLine(" 7) List Families");
        io->printLine(" 8) List Members of a Family");
        io->printLine(" 9) Exit");
        io->printLine("Choice: ");

        std::string line;

        if (!io->getLine(line)) 
        {
            // EOF or error
            break;
        }

        // trim
        line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch){ return !std::isspace(ch); }));
        line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char ch){ return !std::isspace(ch); }).base(), line.end());

        if (line.empty()) continue;

        int choice = 0;
        try {
            choice = std::stoi(line);
        } catch (...) {

            io->printLine("Invalid choice, please enter a number.");
            continue;
        }

        // Validate choice range to avoid undefined enum values.
        if (choice < static_cast<int>(MenuOption::AddFamily) ||
            choice > static_cast<int>(MenuOption::Exit))
        {

            io->printLine("Invalid choice, please pick a valid menu item.");
            continue;
        }

        MenuOption opt = static_cast<MenuOption>(choice);

        switch (opt)
        {
            case MenuOption::AddFamily: 
            {

                io->printLine("Enter family name: ");
                std::string name;

                io->getLine(name);
                if (name.empty()) {

                    io->printLine("Family name cannot be empty.");
                    break;
                }
                addFamily(name);
                break;
            }
            case MenuOption::DeleteFamily: 
            {

                io->printLine("Enter family id to delete: ");
                std::string idstr;

                io->getLine(idstr);
                try {
                    uint64_t id = std::stoull(idstr);
                    deleteFamily(id);
                } catch (...) {

                    io->printLine("Invalid family id.");
                }
                break;
            }
            case MenuOption::AddMember: 
            {






                io->printLine("Enter family id to add member to: ");
                std::string fidstr; io->getLine(fidstr);
                io->printLine("Enter member name: ");
                std::string mname; io->getLine(mname);
                io->printLine("Enter member nickname (optional): ");
                std::string mnick; io->getLine(mnick);
                try {
                    uint64_t fid = std::stoull(fidstr);
                    if (mname.empty()) {

                        io->printLine("Member name cannot be empty.");
                        break;
                    }
                    Member m(mname, mnick);
                    addMember(fid, m);
                } catch (...) {

                    io->printLine("Invalid family id.");
                }
                break;
            }
            case MenuOption::UpdateMember: 
            {






                io->printLine("Enter member id to update: ");
                std::string midstr; io->getLine(midstr);
                io->printLine("Enter new member name: ");
                std::string newname; io->getLine(newname);
                io->printLine("Enter new member nickname: ");
                std::string newnick; io->getLine(newnick);
                try {
                    uint64_t mid = std::stoull(midstr);
                    updateMember(mid, newname, newnick);
                } catch (...) {

                    io->printLine("Invalid member id.");
                }
                break;
            }
            case MenuOption::DeleteMember: 
            {


                io->printLine("Enter member id to delete: ");
                std::string midstr; io->getLine(midstr);
                try {
                    uint64_t mid = std::stoull(midstr);
                    deleteMember(mid);
                } catch (...) {

                    io->printLine("Invalid member id.");
                }
                break;
            }
            case MenuOption::DeleteMultipleMembers: 
            {


                io->printLine("Enter member ids to delete separated by spaces: ");
                std::string idsline; io->getLine(idsline);
                std::istringstream iss(idsline);
                std::vector<uint64_t> ids;
                std::string token;
                bool parse_error = false;
                while (iss >> token) {
                    try {
                        ids.push_back(std::stoull(token));
                    } catch (...) {
                        parse_error = true;
                        break;
                    }
                }
                if (parse_error || ids.empty()) {

                    io->printLine("Invalid input for member ids.");
                    break;
                }
                deleteMembers(ids);
                break;
            }

            case MenuOption::ListFamilies:
            {
                auto families = home_manager.listFamilies();
                if (families.empty())
                {
                    io->printLine("No families found.");
                }
                else
                {
                    io->printLine("Families:");
                    for (const auto& fam : families)
                    {
                        io->printLine("  ID: " + std::to_string(fam.getId()) + " - " + fam.getName());
                    }
                }
                break;
            }

            case MenuOption::ListMembersOfFamily:
            {
                io->printLine("Enter family id to list members: ");
                std::string fidstr;
                io->getLine(fidstr);
                try {
                    uint64_t fid = std::stoull(fidstr);
                    auto members = home_manager.listMembersOfFamily(fid);
                    if (members.empty())
                    {
                        io->printLine("No members found for family " + std::to_string(fid) + ".");
                    }
                    else
                    {
                        io->printLine("Members of family " + std::to_string(fid) + ":");
                        for (const auto& mem : members)
                        {
                            std::string line = "  ID: " + std::to_string(mem.getId()) + " - " + mem.getName();
                            if (!mem.getNickname().empty())
                            {
                                line += " (" + mem.getNickname() + ")";
                            }
                            io->printLine(line);
                        }
                    }
                } catch (...) {
                    io->printLine("Invalid family id.");
                }
                break;
            }

            case MenuOption::Exit:
            {
                running = false;
                break;
            }
            default:
                // This shouldn't be reachable because we validated the range above.

                io->printLine("Unknown choice.");
                break;
        }
    }


    io->printLine("Goodbye.");
}
