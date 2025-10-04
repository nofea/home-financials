#pragma once

#include "commons.hpp"
#include "family.hpp"

class StorageManager 
{
public:
    StorageManager();
    virtual ~StorageManager();

    bool initializeDatabase(const std::string& dbPath);

    bool saveMemberData(const Members& member);
    bool saveFamilyData(const Family& family);

    Member* getMemberData(const uint64_t& member_id);
    Family* getFamilyData(const uint64_t& family_id);

    bool deleteMemberData(const uint64_t& member_id);
    bool deleteFamilyData(const uint64_t& family_id);


private:

    // Connect to the database
    bool connect(const std::string& connectionString);

    // Disconnect from the database
    void disconnect();
};