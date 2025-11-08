#pragma once

#include "commons.hpp"
#include "family.hpp"
#include <string>
#include <cstdint>

// forward-declare sqlite3 from the sqlite3 C API
struct sqlite3;

class StorageManager 
{
public:
    StorageManager();
    virtual ~StorageManager();

    bool initializeDatabase(const std::string& dbPath);

    // Backwards-compatible boolean wrappers are kept; prefer the Ex versions
    bool saveMemberData(const Member& member, const uint64_t family_id);
    bool saveFamilyData(const Family& family);

    // Extended APIs that return a Result code and optionally an out id
    commons::Result saveMemberDataEx(const Member& member, const uint64_t family_id, uint64_t* out_member_id = nullptr);
    commons::Result saveFamilyDataEx(const Family& family, uint64_t* out_family_id = nullptr);

    Member* getMemberData(const uint64_t& member_id);
    Family* getFamilyData(const uint64_t& family_id);

    // Update operations (boolean wrappers)
    bool updateFamilyData(const uint64_t& family_id, const std::string& new_name);
    bool updateMemberData(const uint64_t& member_id, const std::string& new_name, const std::string& new_nickname);

    // Extended delete/update APIs returning Result codes
    commons::Result deleteMemberDataEx(const uint64_t& member_id);
    commons::Result deleteFamilyDataEx(const uint64_t& family_id);

    commons::Result updateFamilyDataEx(const uint64_t& family_id, const std::string& new_name);
    commons::Result updateMemberDataEx(const uint64_t& member_id, const std::string& new_name, const std::string& new_nickname);

    bool deleteMemberData(const uint64_t& member_id);
    bool deleteFamilyData(const uint64_t& family_id);

    // Listing helpers for UI
    std::vector<Family> listFamilies();
    std::vector<Member> listMembersOfFamily(uint64_t family_id);
    
    // Persist a parsed bank-account row into BankAccounts. Returns a
    // commons::Result and optional out id of the inserted BankAccount row.
    commons::Result saveBankAccountEx(uint64_t bank_id,
                                      uint64_t member_id,
                                      const std::string &account_number,
                                      long long opening_paise,
                                      long long closing_paise,
                                      uint64_t* out_id = nullptr);

    // Backwards-compatible boolean wrapper
    bool saveBankAccount(uint64_t bank_id,
                         uint64_t member_id,
                         const std::string &account_number,
                         long long opening_paise,
                         long long closing_paise);
    
    // Efficient helper: return the current number of members in a family.
    //
    // Note on types: SQLite exposes integer results as 64-bit values and many
    // APIs in this codebase use 64-bit IDs/counts for consistency with the DB
    // layer. We therefore return a `uint64_t` here to avoid truncation when
    // reading directly from SQLite. Callers that enforce business rules
    // (e.g. REQ-3 max 255 members) should validate the returned value and
    // cast to a narrower type (for example `uint16_t`) once the domain
    // constraint has been checked.
    //
    // The function returns the raw count; if `out_ok` is non-null it will be
    // set to true on success and false on failure. On failure the returned
    // value is undefined (0 is returned).
    uint64_t getMemberCount(const uint64_t family_id, bool* out_ok = nullptr);

    // Resolve a bank name (case-insensitive) to its Bank_ID. Returns
    // commons::Result::Ok and writes to out_bank_id on success, or
    // commons::Result::NotFound when no match exists.
    commons::Result getBankIdByName(const std::string &bank_name, uint64_t* out_bank_id);

    // Resolve a Bank_ID to its Bank_Name. Returns Ok and writes to out_name
    // when found, NotFound when the id does not exist, or DbError on DB errors.
    commons::Result getBankNameById(const uint64_t bank_id, std::string* out_name);

    // Use the dedicated BankAccount value/type for persisted bank-account rows
    // (defined in `inc/bank_account.hpp`). The class is fully encapsulated
    // and exposes getters/setters for its fields.

    // Retrieve a bank account row by its BankAccount_ID. Returns Ok and
    // populates out_row on success, NotFound when the id does not exist,
    // or DbError on DB errors.
    commons::Result getBankAccountById(const uint64_t bank_account_id, class BankAccount* out_row);

    // List all bank account rows that belong to a member. Returns an empty
    // vector on error or when no accounts exist. Prefer using the
    // Result-returning `getBankAccountById` for single-row access; this is a
    // convenience helper used by higher-level features like NetWorth.
    std::vector<BankAccount> listBankAccountsOfMember(const uint64_t member_id);

private:
    // Owned SQLite connection handle (nullptr when not connected)
    sqlite3* db_handle{nullptr};

    // Whether `connect` has been successfully called and a valid handle is present
    bool connected{false};

    // Connect to the database
    bool connect(const std::string& connectionString);

    // Disconnect from the database
    void disconnect();

    // Ensure DB file and tables exist at the provided path
    void dbInit(const std::string& dbPath);
};