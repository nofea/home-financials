#pragma once

#include "commons.hpp"
#include "storage_manager.hpp"
#include "bank_reader.hpp"
#include <memory>
#include <string>
#include <cstdint>

/**
 * HomeManager acts as an interface between the UI layer and StorageManager.
 * It exposes higher-level operations used by the UI: add/update/delete
 * families and members and simple retrieval helpers.
 */
class HomeManager
{
public:
    HomeManager();
    ~HomeManager();

    // Family operations (return commons::Result for error reporting)
    commons::Result addFamily(const Family &family);
    commons::Result addFamily(const Family &family, uint64_t* out_family_id);
    std::unique_ptr<Family> getFamily(const uint64_t family_id);
    commons::Result updateFamilyName(const uint64_t family_id, const std::string &new_name);
    commons::Result deleteFamily(const uint64_t family_id);

    // Member operations
    commons::Result addMemberToFamily(const Member &member, const uint64_t family_id);
    commons::Result addMemberToFamily(const Member &member, const uint64_t family_id, uint64_t* out_member_id);
    std::unique_ptr<Member> getMember(const uint64_t member_id);
    commons::Result updateMember(const uint64_t member_id, const std::string &new_name, const std::string &new_nickname);
    commons::Result deleteMember(const uint64_t member_id);

    // Listing helpers
    std::vector<Family> listFamilies();
    std::vector<Member> listMembersOfFamily(const uint64_t family_id);

    // Net worth helpers: compute net worth (in paise) for a member or family.
    commons::Result computeMemberNetWorth(const uint64_t member_id, long long* out_net_worth_paise);
    commons::Result computeFamilyNetWorth(const uint64_t family_id, long long* out_net_worth_paise);

    // Testing access
    StorageManager* getStorageManager() { return ptr_storage.get(); }

    // Import a bank statement: parse the file using the provided BankReader
    // and persist the parsed account row for the given member and bank.
    // Overloads accept either a numeric bank_id or a bank name string.
    commons::Result importBankStatement(BankReader &reader,
                                        const std::string &filePath,
                                        const uint64_t member_id,
                                        const uint64_t bank_id,
                                        uint64_t* out_bank_account_id = nullptr);

    commons::Result importBankStatement(BankReader &reader,
                                        const std::string &filePath,
                                        const uint64_t member_id,
                                        const std::string &bank_name,
                                        uint64_t* out_bank_account_id = nullptr);

    // Convenience overloads: accept bank id or bank name and let the
    // HomeManager create the appropriate reader using ReaderFactory.
    commons::Result importBankStatement(const std::string &filePath,
                                        const uint64_t member_id,
                                        const uint64_t bank_id,
                                        uint64_t* out_bank_account_id = nullptr);

    commons::Result importBankStatement(const std::string &filePath,
                                        const uint64_t member_id,
                                        const std::string &bank_name,
                                        uint64_t* out_bank_account_id = nullptr);

private:
    std::unique_ptr<StorageManager> ptr_storage;
};
