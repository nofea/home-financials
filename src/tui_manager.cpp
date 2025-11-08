#include "tui_manager.hpp"
#include "canara_bank_reader.hpp"
#include "reader_factory.hpp"
#include <string>
#include <sstream>
#include <algorithm>
#include <cctype>

/**
 * @brief Construct a new TUIManager::TUIManager object
 * 
 */
TUIManager::TUIManager()
    : io_ptr(std::make_unique<TerminalIO>())
{
}

/**
 * @brief Construct a new TUIManager::TUIManager object
 * 
 * @param io_ptr Pointer to an IOInterface implementation for I/O operations
 */
TUIManager::TUIManager(std::unique_ptr<IOInterface> io_ptr)
    : io_ptr(std::move(io_ptr))
{
}

/**
 * @brief Destroy the TUIManager::TUIManager object
 * 
 */
TUIManager::~TUIManager()
{
}

/**
 * @brief Show an error message to the user
 * 
 * @param res The result containing error information
 */
void TUIManager::showError(const commons::Result& res)
{
    if (res == commons::Result::Ok) 
    {
        return; // nothing to show for success
    }

    io_ptr->printError(errorMessage(res));
}

/**
 * @brief Add a new family (REQ-1)
 * 
 * @param name Name of the family to add
 * @return commons::Result 
 */
commons::Result TUIManager::addFamily(const std::string& name)
{
    Family f(name);
    uint64_t new_id = 0;
    commons::Result res = home_manager.addFamily(f, &new_id);

    if (res != commons::Result::Ok) 
    {
        showError(res);
    } 
    else 
    {
        io_ptr->printLine("Family '" + 
                          name + 
                          "' added successfully. ID: " + 
                          std::to_string(new_id));
    }
    return res;
}

/**
 * @brief Delete a family (REQ-1.1)
 * 
 * @param family_id ID of the family to delete
 * @return commons::Result 
 */
commons::Result TUIManager::deleteFamily(const uint64_t& family_id)
{
    commons::Result res = home_manager.deleteFamily(family_id);

    if (res != commons::Result::Ok) 
    {
        showError(res);
    } 
    else 
    {
        io_ptr->printLine("Family " + 
                           std::to_string(family_id) + 
                           " deleted successfully.");
    }

    return res;
}

/**
 * @brief Add a new member to a family (REQ-2)
 * 
 * @param family_id ID of the family to add the member to
 * @param member The member to add
 * @return commons::Result 
 */
commons::Result TUIManager::addMember(const uint64_t& family_id, const Member& member)
{
    uint64_t new_id = 0;
    commons::Result res = home_manager.addMemberToFamily(member, family_id, &new_id);

    if (res != commons::Result::Ok) 
    {
        showError(res);
    } 
    else 
    {
        io_ptr->printLine("Member '" + member.getName() + 
                          "' added to family " + 
                          std::to_string(family_id) + 
                          ". ID: " + std::to_string(new_id));
    }

    return res;
}

/**
 * @brief Update an existing member's information (REQ-2.1)
 * 
 * @param member_id 
 * @param new_name 
 * @param new_nickname 
 * @return commons::Result 
 */
commons::Result TUIManager::updateMember(const uint64_t& member_id, 
                                         const std::string& new_name, 
                                         const std::string& new_nickname)
{
    commons::Result res = 
        home_manager.updateMember(member_id, new_name, new_nickname);
    
    if (res != commons::Result::Ok) 
    {
        showError(res);
    } 
    else 
    {
        io_ptr->printLine("Member " + 
                          std::to_string(member_id) + 
                          " updated successfully.");
    }
    return res;
}

/**
 * @brief Delete a member from a family (REQ-2.2)
 * 
 * @param member_id ID of the member to delete
 * @return commons::Result 
 */
commons::Result TUIManager::deleteMember(const uint64_t& member_id)
{
    commons::Result res = home_manager.deleteMember(member_id);

    if (res != commons::Result::Ok) 
    {
        showError(res);
    } 
    else 
    {
        io_ptr->printLine("Member " + 
                          std::to_string(member_id) + 
                          " deleted successfully.");
    }

    return res;
}

/**
 * @brief Delete multiple members from a family (REQ-2.2)
 * 
 * @param member_ids IDs of the members to delete
 * @return commons::Result 
 */
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
            io_ptr->printLine("Member " + std::to_string(id) + " deleted.");
        }
    }

    return final_res;
}

/**
 * @brief Start the terminal UI loop
 * 
 */
void TUIManager::run()
{
    // Helper to check non-negative whole numbers (REQ-4, REQ-5)
    auto is_non_negative_whole_number = [](const std::string& s) -> bool 
    {
        if (s.empty())
        {
            return false;
        }

        return std::all_of(s.begin(), s.end(), [](unsigned char ch){ return std::isdigit(ch); });
    };

    // Welcome page
    io_ptr->printLine("Welcome to Home Financials TUI");
    io_ptr->printLine("============================");

    bool running = true;

    while (running) 
    {
        // Display menu
        io_ptr->printLine("");
        io_ptr->printLine("Select an option:");
        io_ptr->printLine(" 1) Add Family");
        io_ptr->printLine(" 2) Delete Family");
        io_ptr->printLine(" 3) Add Member to Family");
        io_ptr->printLine(" 4) Update Member");
        io_ptr->printLine(" 5) Delete Member");
        io_ptr->printLine(" 6) Delete Multiple Members");
        io_ptr->printLine(" 7) List Families");
        io_ptr->printLine(" 8) List Members of a Family");
        io_ptr->printLine(" 9) Import Bank Statement for a Member");
        io_ptr->printLine("10) Compute Member Net Worth");
        io_ptr->printLine("11) Compute Family Net Worth");
        io_ptr->printLine("12) Exit");
        io_ptr->printLine("Choice: ");

        std::string line;

        if (!io_ptr->getLine(line)) 
        {
            // EOF or error
            break;
        }

        // trim
        line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch)
            { return !std::isspace(ch); }));

        line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char ch)
            { return !std::isspace(ch); }).base(), line.end());

        if (line.empty()) 
        {
            continue;
        }

        int choice = 0;

        try 
        {
            choice = std::stoi(line);
        } 
        catch (...) 
        {

            io_ptr->printLine("Invalid choice, please enter a number.");
            continue;
        }

        // Validate choice range to avoid undefined enum values.
        if (choice < static_cast<int>(MenuOption::AddFamily) ||
            choice > static_cast<int>(MenuOption::Exit))
        {

            io_ptr->printLine("Invalid choice, please pick a valid menu item.");
            continue;
        }

        MenuOption opt = static_cast<MenuOption>(choice);

        switch (opt)
        {
            case MenuOption::AddFamily: 
            {
                io_ptr->printLine("Enter family name: ");
                std::string name;

                io_ptr->getLine(name);

                if (name.empty()) 
                {
                    io_ptr->printLine("Family name cannot be empty.");
                    break;
                }

                addFamily(name);

                break;
            }

            case MenuOption::DeleteFamily: 
            {
                io_ptr->printLine("Enter family id to delete: ");
                std::string idstr;

                io_ptr->getLine(idstr);

                if (!is_non_negative_whole_number(idstr))
                {
                    // REQ-4 and REQ-5 enforcement
                    io_ptr->printLine("Family id must be a non-negative whole number (REQ-4, REQ-5).");
                    // Preserve original generic message for existing tests
                    io_ptr->printLine("Invalid family id.");
                    break;
                }

                try 
                {
                    uint64_t id = std::stoull(idstr);
                    deleteFamily(id);
                } 
                catch (...) 
                {
                    io_ptr->printLine("Invalid family id.");
                }

                break;
            }

            case MenuOption::AddMember: 
            {
                io_ptr->printLine("Enter family id to add member to: ");
                std::string fidstr; io_ptr->getLine(fidstr);
                io_ptr->printLine("Enter member name: ");
                std::string mname; io_ptr->getLine(mname);
                io_ptr->printLine("Enter member nickname (optional): ");
                std::string mnick; io_ptr->getLine(mnick);

                if (!is_non_negative_whole_number(fidstr))
                {
                    // REQ-4 and REQ-5 enforcement
                    io_ptr->printLine("Family id must be a non-negative whole number (REQ-4, REQ-5).");
                    io_ptr->printLine("Invalid family id.");
                    break;
                }

                try 
                {
                    uint64_t fid = std::stoull(fidstr);

                    if (mname.empty()) 
                    {
                        io_ptr->printLine("Member name cannot be empty.");
                        break;
                    }

                    Member m(mname, mnick);
                    addMember(fid, m);

                } 
                catch (...) 
                {
                    io_ptr->printLine("Invalid family id.");
                }
                break;
            }

            case MenuOption::UpdateMember: 
            {

                io_ptr->printLine("Enter member id to update: ");
                std::string midstr; io_ptr->getLine(midstr);
                io_ptr->printLine("Enter new member name: ");
                std::string newname; io_ptr->getLine(newname);
                io_ptr->printLine("Enter new member nickname: ");
                std::string newnick; io_ptr->getLine(newnick);

                if (!is_non_negative_whole_number(midstr))
                {
                    io_ptr->printLine("Member id must be a non-negative whole number (REQ-4, REQ-5).");
                    io_ptr->printLine("Invalid member id.");
                    break;
                }

                try 
                {
                    uint64_t mid = std::stoull(midstr);
                    updateMember(mid, newname, newnick);
                } 
                catch (...) 
                {
                    io_ptr->printLine("Invalid member id.");
                }

                break;
            }

            case MenuOption::DeleteMember: 
            {
                io_ptr->printLine("Enter member id to delete: ");
                std::string midstr; io_ptr->getLine(midstr);

                if (!is_non_negative_whole_number(midstr))
                {
                    io_ptr->printLine("Member id must be a non-negative whole number (REQ-4, REQ-5).");
                    io_ptr->printLine("Invalid member id.");
                    break;
                }

                try 
                {
                    uint64_t mid = std::stoull(midstr);
                    deleteMember(mid);
                } 
                catch (...) 
                {
                    io_ptr->printLine("Invalid member id.");
                }

                break;
            }

            case MenuOption::DeleteMultipleMembers: 
            {
                io_ptr->printLine("Enter member ids to delete separated by spaces: ");
                std::string idsline; io_ptr->getLine(idsline);
                std::istringstream iss(idsline);
                std::vector<uint64_t> ids;
                std::string token;
                bool parse_error = false;

                while (iss >> token) 
                {
                    if (!is_non_negative_whole_number(token))
                    {
                        parse_error = true;
                        break;
                    }
                    try 
                    {
                        ids.push_back(std::stoull(token));
                    } 
                    catch (...) 
                    {
                        parse_error = true;
                        break;
                    }
                }

                if (parse_error || ids.empty()) 
                {
                    io_ptr->printLine("Member ids must be non-negative whole numbers (REQ-4, REQ-5).");
                    io_ptr->printLine("Invalid input for member ids.");
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
                    io_ptr->printLine("No families found.");
                }
                else
                {
                    io_ptr->printLine("Families:");
                    for (const auto& fam : families)
                    {
                        io_ptr->printLine("  ID: " + std::to_string(fam.getId()) + " - " + fam.getName());
                    }
                }

                break;
            }

            case MenuOption::ListMembersOfFamily:
            {
                io_ptr->printLine("Enter family id to list members: ");
                std::string fidstr;
                io_ptr->getLine(fidstr);

                if (!is_non_negative_whole_number(fidstr))
                {
                    io_ptr->printLine("Family id must be a non-negative whole number (REQ-4, REQ-5).");
                    io_ptr->printLine("Invalid family id.");
                    break;
                }

                try 
                {
                    uint64_t fid = std::stoull(fidstr);
                    auto members = home_manager.listMembersOfFamily(fid);

                    if (members.empty())
                    {
                        io_ptr->printLine("No members found for family " + std::to_string(fid) + ".");
                    }
                    else
                    {
                        io_ptr->printLine("Members of family " + std::to_string(fid) + ":");

                        for (const auto& mem : members)
                        {
                            std::string line = "  ID: " + std::to_string(mem.getId()) + " - " + mem.getName();

                            if (!mem.getNickname().empty())
                            {
                                line += " (" + mem.getNickname() + ")";
                            }

                            io_ptr->printLine(line);
                        }
                    }
                } 
                catch (...) 
                {
                    io_ptr->printLine("Invalid family id.");
                }

                break;
            }
            
            case MenuOption::ImportBankStatement:
            {
                // Flow: ask for member id, bank (id or name), and file path
                io_ptr->printLine("Enter member id to attach the account to: ");
                std::string memberIdStr; io_ptr->getLine(memberIdStr);

                if (!is_non_negative_whole_number(memberIdStr))
                {
                    io_ptr->printLine("Member id must be a non-negative whole number (REQ-4, REQ-5).");
                    io_ptr->printLine("Invalid member id.");
                    break;
                }

                uint64_t memberId = 0;
                try {
                    memberId = std::stoull(memberIdStr);
                } catch (...) {
                    io_ptr->printLine("Invalid member id.");
                    break;
                }

                // Show supported/registered readers to help the user choose
                auto registeredReaders = ReaderFactory::listRegistered();
                if (!registeredReaders.empty())
                {
                    std::string banksLine = "Supported banks: ";
                    for (size_t idx = 0; idx < registeredReaders.size(); ++idx)
                    {
                        std::string displayName = registeredReaders[idx];
                        if (!displayName.empty())
                        {
                            // Capitalize first letter for nicer display
                            displayName[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(displayName[0])));
                        }
                        banksLine += displayName;
                        if (idx + 1 < registeredReaders.size())
                        {
                            banksLine += ", ";
                        }
                    }
                    io_ptr->printLine(banksLine);
                }

                io_ptr->printLine("Enter bank id or name (e.g. Canara): ");
                std::string bankInput; io_ptr->getLine(bankInput);
                if (bankInput.empty()) {
                    io_ptr->printLine("Bank id/name cannot be empty.");
                    break;
                }

                io_ptr->printLine("Enter path to statement file (CSV): ");
                std::string path; io_ptr->getLine(path);
                if (path.empty()) {
                    io_ptr->printLine("File path cannot be empty.");
                    break;
                }

                // Decide whether bankInput is numeric (id) or name
                bool isNumeric = std::all_of(bankInput.begin(), bankInput.end(), [](unsigned char ch){ return std::isdigit(ch); });

                commons::Result res = commons::Result::InvalidInput;
                uint64_t outBankAccountId = 0;

                if (isNumeric) {
                    try {
                        uint64_t bankId = std::stoull(bankInput);
                        // Use HomeManager convenience overload which creates reader via ReaderFactory
                        res = home_manager.importBankStatement(path, memberId, bankId, &outBankAccountId);
                    } catch (...) {
                        io_ptr->printLine("Invalid bank id.");
                        break;
                    }
                } else {
                    // bankInput treated as name
                    // Let HomeManager and ReaderFactory handle selecting the reader by name
                    res = home_manager.importBankStatement(path, memberId, bankInput, &outBankAccountId);
                }

                if (res != commons::Result::Ok) {
                    showError(res);
                } else {
                    io_ptr->printLine("Bank account imported successfully. ID: " + std::to_string(outBankAccountId));
                }

                break;
            }

            case MenuOption::ComputeMemberNetWorth:
            {
                io_ptr->printLine("Enter member id to compute net worth: ");
                std::string midstr;
                io_ptr->getLine(midstr);

                if (!is_non_negative_whole_number(midstr))
                {
                    io_ptr->printLine("Member id must be a non-negative whole number (REQ-4, REQ-5).\nInvalid member id.");
                    break;
                }

                uint64_t mid = 0;

                try
                {
                    mid = std::stoull(midstr);
                }
                catch (...)
                {
                    io_ptr->printLine("Invalid member id.");
                    break;
                }

                // Compute net worth via HomeManager API
                long long net_paise = 0;
                commons::Result res = home_manager.computeMemberNetWorth(mid, &net_paise);

                if (res != commons::Result::Ok)
                {
                    showError(res);
                    break;
                }

                // Format as rupees.paise (two digits)
                long long rupees = net_paise / 100;
                int paise = static_cast<int>(std::llabs(net_paise % 100));
                std::string sign = net_paise < 0 ? "-" : "";
                std::ostringstream oss;
                oss << "Member " << mid << " net worth: " << sign << rupees << ".";
                if (paise < 10) oss << "0" << paise; else oss << paise;
                io_ptr->printLine(oss.str());

                break;
            }

            case MenuOption::ComputeFamilyNetWorth:
            {
                io_ptr->printLine("Enter family id to compute net worth: ");
                std::string fidstr;
                io_ptr->getLine(fidstr);

                if (!is_non_negative_whole_number(fidstr))
                {
                    io_ptr->printLine("Family id must be a non-negative whole number (REQ-4, REQ-5).\nInvalid family id.");
                    break;
                }

                uint64_t fid = 0;

                try
                {
                    fid = std::stoull(fidstr);
                }
                catch (...)
                {
                    io_ptr->printLine("Invalid family id.");
                    break;
                }

                long long family_paise = 0;
                commons::Result fres = home_manager.computeFamilyNetWorth(fid, &family_paise);

                if (fres != commons::Result::Ok)
                {
                    showError(fres);
                    break;
                }

                long long rupees = family_paise / 100;
                int paise = static_cast<int>(std::llabs(family_paise % 100));
                std::string sign = family_paise < 0 ? "-" : "";
                std::ostringstream oss;
                oss << "Family " << fid << " net worth: " << sign << rupees << ".";
                if (paise < 10) oss << "0" << paise; else oss << paise;
                io_ptr->printLine(oss.str());

                break;
            }

            case MenuOption::Exit:
            {
                running = false;
                break;
            }

            default:
            {
                // This shouldn't be reachable because we validated the range above.

                io_ptr->printLine("Unknown choice.");
                break;
            }
                
        }
    }

    io_ptr->printLine("Goodbye.");
}
