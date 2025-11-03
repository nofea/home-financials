#include "storage_manager.hpp"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <sqlite3.h>

// Database initialization is handled by StorageManager::runDbInitScript().

int main()
{
    StorageManager storageManager;
    // Ensure DB initialization runs before application logic.
    storageManager.initializeDatabase("");

    // First create and save the family
    Family family("Doe Family");
    if (!storageManager.saveFamilyData(family)) 
    {
        std::cerr << "Failed to save family" << std::endl;
        return 1;
    }

    // Get the family ID from the database (in practice, you'd track this after saving)
    sqlite3* db = nullptr;
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT Family_ID FROM FamilyInfo WHERE Family_Name = ?;";
    
    int ret_code = sqlite3_open("homefinancials.db", &db);
    if (ret_code != SQLITE_OK) 
    {
        std::cerr << "Failed to open database" << std::endl;
        return 1;
    }
    
    ret_code = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (ret_code != SQLITE_OK) 
    {
        std::cerr << "Failed to prepare statement" << std::endl;
        sqlite3_close(db);
        return 1;
    }
    
    sqlite3_bind_text(stmt, 1, "Doe Family", -1, SQLITE_TRANSIENT);
    uint64_t family_id = 0;
    
    if (sqlite3_step(stmt) == SQLITE_ROW) 
    {
        family_id = sqlite3_column_int64(stmt, 0);
    }
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    
    if (family_id == 0) 
    {
        std::cerr << "Failed to get family ID" << std::endl;
        return 1;
    }

    // Now create and save a member in that family
    Member member1("John Doe", "Johnny");
    if (!storageManager.saveMemberData(member1, family_id)) 
    {
        std::cerr << "Failed to save member" << std::endl;
        return 1;
    }

    return 0;
}