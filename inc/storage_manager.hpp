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

    bool saveMemberData(const Member& member, const uint64_t family_id);
    bool saveFamilyData(const Family& family);

    Member* getMemberData(const uint64_t& member_id);
    Family* getFamilyData(const uint64_t& family_id);

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
    void runDbInitScript(const std::string& dbPath);
};