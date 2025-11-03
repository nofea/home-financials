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

    // Simple result codes for operations
    enum class Result {
        Ok = 0,
        InvalidInput = 1,
        NotFound = 2,
        DbError = 3,
    };

    // Backwards-compatible boolean wrappers are kept; prefer the Ex versions
    bool saveMemberData(const Member& member, const uint64_t family_id);
    bool saveFamilyData(const Family& family);

    // Extended APIs that return a Result code and optionally an out id
    Result saveMemberDataEx(const Member& member, const uint64_t family_id, uint64_t* out_member_id = nullptr);
    Result saveFamilyDataEx(const Family& family, uint64_t* out_family_id = nullptr);

    Member* getMemberData(const uint64_t& member_id);
    Family* getFamilyData(const uint64_t& family_id);

    // Update operations (boolean wrappers)
    bool updateFamilyData(const uint64_t& family_id, const std::string& new_name);
    bool updateMemberData(const uint64_t& member_id, const std::string& new_name, const std::string& new_nickname);

    // Extended delete/update APIs returning Result codes
    Result deleteMemberDataEx(const uint64_t& member_id);
    Result deleteFamilyDataEx(const uint64_t& family_id);

    Result updateFamilyDataEx(const uint64_t& family_id, const std::string& new_name);
    Result updateMemberDataEx(const uint64_t& member_id, const std::string& new_name, const std::string& new_nickname);

    bool deleteMemberData(const uint64_t& member_id);
    bool deleteFamilyData(const uint64_t& family_id);


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