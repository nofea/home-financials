#include "storage_manager.hpp"
#include "bank_account.hpp"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include <sqlite3.h>

/**
 * @brief Construct a new Storage Manager:: Storage Manager object
 */
StorageManager::StorageManager() 
{
}

/**
 * @brief Destroy the Storage Manager:: Storage Manager object
 */
StorageManager::~StorageManager() 
{
    // Ensure DB handle is closed on destruction
    disconnect();
}

/**
 * @brief Initialize the database at the given path.
 * 
 * @param dbPath Path to the database file.
 * @return true If initialization was successful.
 * @return false If initialization failed.
 */
bool StorageManager::initializeDatabase(const std::string& dbPath) 
{
    // Determine database path. If caller provided a path (non-empty and not the
    // placeholder), use it. Otherwise, try to infer project root and use
    // project_root/homefinancials.db
    std::string chosenPath = dbPath;

    if (chosenPath.empty() || chosenPath == "path/to/database") 
    {
        namespace fs = std::filesystem;
        fs::path projectRoot;

        try 
        {
            fs::path exe = fs::read_symlink("/proc/self/exe");
            projectRoot = exe.parent_path().parent_path();
            if (!fs::exists(projectRoot)) {
                projectRoot = exe.parent_path();
            }
        } 
        catch (...) 
        {
            projectRoot = fs::current_path();
        }

        fs::path dbPathFs = projectRoot / "homefinancials.db";
        chosenPath = dbPathFs.string();
    }

    // Ensure tables exist at the chosen path
    dbInit(chosenPath);

    // Try to connect to the database
    return connect(chosenPath);
}

/**
 * @brief Initialize DB file and tables at the provided path.
 * 
 * @param dbPathStr Path to the database file.
 */
void StorageManager::dbInit(const std::string& dbPathStr)
{
    namespace fs = std::filesystem;
    fs::path dbPath = fs::path(dbPathStr);
    fs::path projectRoot = dbPath.parent_path();

    std::cout << "Initializing SQLite DB at: " << dbPath << std::endl;

    // Create parent directories if needed
    try 
    {
        if (!projectRoot.empty() && !fs::exists(projectRoot)) 
        {
            fs::create_directories(projectRoot);
        }
    } 
    catch (const std::exception &e) 
    {
        std::cerr << "Failed to create DB parent directories: " << e.what() << std::endl;
    }

    // SQL statements to create only the required tables: FamilyInfo and MemberInfo
    const std::vector<std::pair<const char*, const char*>> table_ddls = 
    {
        {
            "FamilyInfo", R"(
            CREATE TABLE IF NOT EXISTS FamilyInfo (
            Family_ID INTEGER PRIMARY KEY AUTOINCREMENT,
            Family_Name TEXT NOT NULL
            );
            )"
        },

        {
            "MemberInfo", R"(
            CREATE TABLE IF NOT EXISTS MemberInfo (
            Member_ID INTEGER PRIMARY KEY AUTOINCREMENT,
            Family_ID INTEGER NOT NULL,
            Member_Name TEXT NOT NULL,
            Member_Nick_Name TEXT,
            FOREIGN KEY(Family_ID) REFERENCES FamilyInfo(Family_ID) ON DELETE CASCADE
            );
            )"
        }
        ,
        {
            "BankList", R"(
            CREATE TABLE IF NOT EXISTS BankList (
            Bank_ID INTEGER PRIMARY KEY AUTOINCREMENT,
            Bank_Name TEXT NOT NULL UNIQUE
            );
            )"
        },

        {
            "BankAccounts", R"(
            CREATE TABLE IF NOT EXISTS BankAccounts (
            BankAccount_ID INTEGER PRIMARY KEY AUTOINCREMENT,
            Bank_ID INTEGER NOT NULL,
            Member_ID INTEGER NOT NULL,
            Account_Number TEXT NOT NULL,
            Opening_Balance INTEGER NOT NULL,
            Closing_Balance INTEGER NOT NULL,
            FOREIGN KEY(Bank_ID) REFERENCES BankList(Bank_ID),
            FOREIGN KEY(Member_ID) REFERENCES MemberInfo(Member_ID) ON DELETE CASCADE
            );
            )"
        }
    };

    sqlite3* db = nullptr;
    int ret_code = sqlite3_open(dbPath.string().c_str(), &db);

    if (ret_code != SQLITE_OK) 
    {
        std::cerr << "Cannot open SQLite DB: " << (db ? sqlite3_errmsg(db) : "(no handle)") << std::endl;
        if (db) sqlite3_close(db);
        return;
    }

    // Enable foreign key enforcement
    char* errmsg = nullptr;
    ret_code = sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, &errmsg);

    if (ret_code != SQLITE_OK) 
    {
        std::cerr << "Failed to enable foreign keys: " << (errmsg ? errmsg : "") << std::endl;
        sqlite3_free(errmsg);
    }

    for (const auto &p : table_ddls) 
    {
        ret_code = sqlite3_exec(db, p.second, nullptr, nullptr, &errmsg);

        if (ret_code != SQLITE_OK) 
        {
            std::cerr << "Error creating table '" << p.first << "': " << (errmsg ? errmsg : "") << std::endl;
            sqlite3_free(errmsg);
        } 
        else 
        {
            std::cout << "Checked/created table: " << p.first << std::endl;
        }
    }

    // Prepopulate BankList with known banks if it's empty.
    const char* countBanksSql = "SELECT COUNT(1) FROM BankList;";
    sqlite3_stmt* countStmt = nullptr;
    ret_code = sqlite3_prepare_v2(db, countBanksSql, -1, &countStmt, nullptr);

    if (ret_code == SQLITE_OK)
    {
        ret_code = sqlite3_step(countStmt);

        if (ret_code == SQLITE_ROW)
        {
            sqlite3_int64 bankCount = sqlite3_column_int64(countStmt, 0);
            if (bankCount == 0)
            {
                const char* insertSql = "INSERT INTO BankList (Bank_Name) VALUES (?);";
                sqlite3_stmt* insStmt = nullptr;
                const char* banks[] = {"Canara", "SBI", "Axis", "HDFC", "PNB"};

                for (const char* bn : banks)
                {
                    // Ensure stmt pointer does not carry over from previous iteration
                    insStmt = nullptr;
                    if (sqlite3_prepare_v2(db, insertSql, -1, &insStmt, nullptr) == SQLITE_OK)
                    {
                        sqlite3_bind_text(insStmt, 1, bn, -1, SQLITE_TRANSIENT);
                        ret_code = sqlite3_step(insStmt);
                        
                        if (ret_code != SQLITE_DONE)
                        {
                            std::cerr << "Failed to insert bank '" << bn << "': " << sqlite3_errmsg(db) << std::endl;
                        }
                        sqlite3_finalize(insStmt);
                    }
                    else
                    {
                        std::cerr << "Failed to prepare insert for bank '" << bn << "': " << sqlite3_errmsg(db) << std::endl;
                    }
                }
            }
        }
        sqlite3_finalize(countStmt);
    }

    sqlite3_close(db);
}

/**
 * @brief Save member data to the database.
 * 
 * @param member Member to save.
 * @param family_id ID of the family the member belongs to.
 * @return true if save was successful.
 * @return false if save failed.
 */
bool StorageManager::saveMemberData(const Member& member, const uint64_t family_id) 
{
    return saveMemberDataEx(member, family_id, nullptr) == commons::Result::Ok;
}

/**
 * @brief Save family data to the database.
 * 
 * @param family family
 * @return true if save was successful.
 * @return false if save failed.
 */
bool StorageManager::saveFamilyData(const Family& family) 
{
    return saveFamilyDataEx(family, nullptr) == commons::Result::Ok;
}

/**
 * @brief Get member data by ID.
 * 
 * @param member_id Member ID to retrieve.
 * @return Member* 
 */
Member* StorageManager::getMemberData(const uint64_t& member_id) 
{
    if (!connected) 
    {
        if (!initializeDatabase(""))
        {
            return nullptr;
        } 
    }

    const char* sql = "SELECT Member_ID, Family_ID, Member_Name, Member_Nick_Name FROM MemberInfo WHERE Member_ID = ?;";
    sqlite3_stmt* stmt = nullptr;
    int ret_code = sqlite3_prepare_v2(db_handle, sql, -1, &stmt, nullptr);

    if (ret_code != SQLITE_OK) 
    {
        std::cerr << "Failed to prepare select member: " << sqlite3_errmsg(db_handle) << std::endl;
        return nullptr;
    }

    sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(member_id));
    ret_code = sqlite3_step(stmt);

    if (ret_code == SQLITE_ROW) 
    {
        const unsigned char* name = sqlite3_column_text(stmt, 2);
        const unsigned char* nick = sqlite3_column_text(stmt, 3);
        std::string sname = name ? reinterpret_cast<const char*>(name) : std::string();
        std::string snick = nick ? reinterpret_cast<const char*>(nick) : std::string();
        Member* m = new Member(sname, snick);
        // Note: Member::member_id cannot be set (private). Caller may rely on DB ids separately.
        sqlite3_finalize(stmt);
        return m;
    }

    sqlite3_finalize(stmt);
    return nullptr;
}

/**
 * @brief Get family data by ID.
 * 
 * @param family_id ID of the family to retrieve.
 * @return Family* 
 */
Family* StorageManager::getFamilyData(const uint64_t& family_id) 
{
    if (!connected) 
    {
        if (!initializeDatabase(""))
        {
            return nullptr;
        }
    }

    const char* fsql = "SELECT Family_ID, Family_Name FROM FamilyInfo WHERE Family_ID = ?;";
    sqlite3_stmt* fstmt = nullptr;

    int ret_code = sqlite3_prepare_v2(db_handle, fsql, -1, &fstmt, nullptr);
    
    if (ret_code != SQLITE_OK) 
    {
        std::cerr << "Failed to prepare select family: " << sqlite3_errmsg(db_handle) << std::endl;
        return nullptr;
    }

    sqlite3_bind_int64(fstmt, 1, static_cast<sqlite3_int64>(family_id));
    ret_code = sqlite3_step(fstmt);

    if (ret_code != SQLITE_ROW) 
    {
        sqlite3_finalize(fstmt);
        return nullptr;
    }

    const unsigned char* fname = sqlite3_column_text(fstmt, 1);
    std::string sname = fname ? reinterpret_cast<const char*>(fname) : std::string();
    Family* family = new Family(sname);
    sqlite3_finalize(fstmt);

    // Load members
    const char* msql = "SELECT Member_ID, Member_Name, Member_Nick_Name FROM MemberInfo WHERE Family_ID = ?;";
    sqlite3_stmt* mstmt = nullptr;
    ret_code = sqlite3_prepare_v2(db_handle, msql, -1, &mstmt, nullptr);

    if (ret_code == SQLITE_OK) 
    {
        sqlite3_bind_int64(mstmt, 1, static_cast<sqlite3_int64>(family_id));

        while ((ret_code = sqlite3_step(mstmt)) == SQLITE_ROW) 
        {
            const unsigned char* mname = sqlite3_column_text(mstmt, 1);
            const unsigned char* mnick = sqlite3_column_text(mstmt, 2);
            std::string ms = mname ? reinterpret_cast<const char*>(mname) : std::string();
            std::string mns = mnick ? reinterpret_cast<const char*>(mnick) : std::string();
            Member m(ms, mns);
            family->addMember(m);
        }

        sqlite3_finalize(mstmt);
    }

    return family;
}

/**
 * @brief Delete member data by ID.
 * 
 * @param member_id ID of the member to delete.
 * @return true if deletion was successful.
 * @return false if deletion failed.
 */
bool StorageManager::deleteMemberData(const uint64_t& member_id) 
{
    if (!connected) 
    {
        if (!initializeDatabase("")) 
        {
            return false;
        }
    }

    const char* sql = "DELETE FROM MemberInfo WHERE Member_ID = ?;";
    sqlite3_stmt* stmt = nullptr;
    int ret_code = sqlite3_prepare_v2(db_handle, sql, -1, &stmt, nullptr);

    if (ret_code != SQLITE_OK) 
    {
        std::cerr << "Failed to prepare delete member: " << sqlite3_errmsg(db_handle) << std::endl;
        return false;
    }

    sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(member_id));
    ret_code = sqlite3_step(stmt);
    bool success = (ret_code == SQLITE_DONE && sqlite3_changes(db_handle) > 0);
    sqlite3_finalize(stmt);

    return success;
}

/**
 * @brief Delete family data by ID.
 * 
 * @param family_id ID of the family to delete.
 * @return true if deletion was successful.
 * @return false if deletion failed.
 */
bool StorageManager::deleteFamilyData(const uint64_t& family_id) 
{
    if (!connected) 
    {
        if (!initializeDatabase(""))
        {
            return false;
        }
    }

    const char* sql = "DELETE FROM FamilyInfo WHERE Family_ID = ?;";
    sqlite3_stmt* stmt = nullptr;
    int ret_code = sqlite3_prepare_v2(db_handle, sql, -1, &stmt, nullptr);

    if (ret_code != SQLITE_OK) 
    {
        std::cerr << "Failed to prepare delete family: " << sqlite3_errmsg(db_handle) << std::endl;
        return false;
    }

    sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(family_id));
    ret_code = sqlite3_step(stmt);
    bool success = (ret_code == SQLITE_DONE && sqlite3_changes(db_handle) > 0);
    sqlite3_finalize(stmt);

    return success;
}

/**
 * @brief Update family data.
 * 
 * @param family_id ID of the family to update.
 * @param new_name Name to update to.
 * @return true if update was successful.
 * @return false if update failed.
 */
bool StorageManager::updateFamilyData(const uint64_t& family_id, const std::string& new_name)
{
    if (!connected) 
    {
        if (!initializeDatabase("")) 
        {
            return false;
        }
    }

    const char* sql = "UPDATE FamilyInfo SET Family_Name = ? WHERE Family_ID = ?;";
    sqlite3_stmt* stmt = nullptr;
    int ret_code = sqlite3_prepare_v2(db_handle, sql, -1, &stmt, nullptr);

    if (ret_code != SQLITE_OK) 
    {
        std::cerr << "Failed to prepare update family: " << sqlite3_errmsg(db_handle) << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, new_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, static_cast<sqlite3_int64>(family_id));
    ret_code = sqlite3_step(stmt);
    bool success = (ret_code == SQLITE_DONE && sqlite3_changes(db_handle) > 0);
    sqlite3_finalize(stmt);

    return success;
}

/**
 * @brief Update member data.
 * 
 * @param member_id ID of the member to update.
 * @param new_name Name to update to.
 * @param new_nickname Nickname to update to.
 * @return true 
 * @return false 
 */
bool StorageManager::updateMemberData(const uint64_t& member_id, const std::string& new_name, const std::string& new_nickname)
{
    // Delegate to Ex version, reuse logic but convert commons::Result to bool
    auto result = updateMemberDataEx(member_id, new_name, new_nickname);
    return result == commons::Result::Ok;
}

/**
 * @brief Save family data to the database.
 * 
 * @param family Family to save.
 * @param out_family_id ID output parameter.
 * @return commons::Result 
 */
commons::Result StorageManager::saveFamilyDataEx(const Family& family, uint64_t* out_family_id)
{
    if (family.getName().empty()) 
    {
        return commons::Result::InvalidInput;
    }

    if (!connected) 
    {
        if (!initializeDatabase(""))
        {
            return commons::Result::DbError;
        }
    }

    const char* sql = "INSERT INTO FamilyInfo (Family_Name) VALUES (?);";
    sqlite3_stmt* stmt = nullptr;
    int ret_code = sqlite3_prepare_v2(db_handle, sql, -1, &stmt, nullptr);

    if (ret_code != SQLITE_OK) 
    {
        return commons::Result::DbError;
    }

    ret_code = sqlite3_bind_text(stmt, 1, family.getName().c_str(), -1, SQLITE_TRANSIENT);
    
    if (ret_code != SQLITE_OK) 
    {
        sqlite3_finalize(stmt);
        return commons::Result::DbError;
    }

    ret_code = sqlite3_step(stmt);

    if (ret_code != SQLITE_DONE) 
    {
        sqlite3_finalize(stmt);
        return commons::Result::DbError;
    }

    sqlite3_finalize(stmt);
    sqlite3_int64 family_id = sqlite3_last_insert_rowid(db_handle);

    // Insert members if any
    auto members = family.getMembers();

    for (const auto &m : members) 
    {
        const char* msql = "INSERT INTO MemberInfo (Family_ID, Member_Name, Member_Nick_Name) VALUES (?, ?, ?);";
        sqlite3_stmt* mstmt = nullptr;
        ret_code = sqlite3_prepare_v2(db_handle, msql, -1, &mstmt, nullptr);

        if (ret_code != SQLITE_OK) 
        {
            // continue inserting others but mark DB error
            sqlite3_finalize(mstmt);
            continue;
        }

        sqlite3_bind_int64(mstmt, 1, family_id);
        sqlite3_bind_text(mstmt, 2, m.getName().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(mstmt, 3, m.getNickname().c_str(), -1, SQLITE_TRANSIENT);
        ret_code = sqlite3_step(mstmt);
        sqlite3_finalize(mstmt);
    }

    if (out_family_id) *out_family_id = static_cast<uint64_t>(family_id);
    return commons::Result::Ok;
}

/**
 * @brief Save member data to the database.
 * 
 * @param member Member to save.
 * @param family_id ID of the family the member belongs to.
 * @param out_member_id Member ID output parameter.
 * @return commons::Result 
 */
commons::Result StorageManager::saveMemberDataEx(const Member& member, const uint64_t family_id, uint64_t* out_member_id)
{
    if (member.getName().empty())
    {
        return commons::Result::InvalidInput;
    }

    if (family_id == 0)
    {
        return commons::Result::InvalidInput;
    } 

    if (!connected) 
    {
        if (!initializeDatabase(""))
        {
            return commons::Result::DbError;
        }
    }

    // Check family exists
    const char* check_sql = "SELECT 1 FROM FamilyInfo WHERE Family_ID = ?;";
    sqlite3_stmt* check_stmt = nullptr;
    int ret_code = sqlite3_prepare_v2(db_handle, check_sql, -1, &check_stmt, nullptr);

    if (ret_code != SQLITE_OK)
    {
        return commons::Result::DbError;
    } 

    sqlite3_bind_int64(check_stmt, 1, static_cast<sqlite3_int64>(family_id));
    ret_code = sqlite3_step(check_stmt);
    sqlite3_finalize(check_stmt);

    if (ret_code != SQLITE_ROW)
    {
        return commons::Result::NotFound;
    } 

    // Check current number of members in the family to enforce REQ-3 (max 255 members)
    bool ok = false;
    uint64_t current_count = getMemberCount(family_id, &ok);
    if (!ok)
    {
        return commons::Result::DbError;
    }
    if (current_count >= 255)
    {
        return commons::Result::MaxMembersExceeded;
    }

    const char* sql = "INSERT INTO MemberInfo (Family_ID, Member_Name, Member_Nick_Name) VALUES (?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    ret_code = sqlite3_prepare_v2(db_handle, sql, -1, &stmt, nullptr);

    if (ret_code != SQLITE_OK)
    {
        return commons::Result::DbError;
    }
    

    sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(family_id));
    sqlite3_bind_text(stmt, 2, member.getName().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, member.getNickname().c_str(), -1, SQLITE_TRANSIENT);

    ret_code = sqlite3_step(stmt);

    if (ret_code != SQLITE_DONE) 
    {
        sqlite3_finalize(stmt);
        return commons::Result::DbError;
    }

    sqlite3_finalize(stmt);

    if (out_member_id)
    {
        *out_member_id = static_cast<uint64_t>(sqlite3_last_insert_rowid(db_handle));
    } 

    return commons::Result::Ok;
}

/**
 * @brief Save a parsed bank account row into BankAccounts.
 *
 * This validates that the referenced Bank and Member exist, then inserts
 * the row. Balances are expected in paise (integer cents).
 */
commons::Result StorageManager::saveBankAccountEx(uint64_t bank_id,
                                                 uint64_t member_id,
                                                 const std::string &account_number,
                                                 long long opening_paise,
                                                 long long closing_paise,
                                                 uint64_t* out_id)
{
    if (account_number.empty())
    {
        return commons::Result::InvalidInput;
    }

    if (!connected)
    {
        if (!initializeDatabase(""))
        {
            return commons::Result::DbError;
        }
    }

    // Ensure bank exists
    const char* bank_check_sql = "SELECT 1 FROM BankList WHERE Bank_ID = ?;";
    sqlite3_stmt* bank_stmt = nullptr;
    int ret_code = sqlite3_prepare_v2(db_handle, bank_check_sql, -1, &bank_stmt, nullptr);
    if (ret_code != SQLITE_OK)
    {
        if (bank_stmt) sqlite3_finalize(bank_stmt);
        return commons::Result::DbError;
    }

    sqlite3_bind_int64(bank_stmt, 1, static_cast<sqlite3_int64>(bank_id));
    ret_code = sqlite3_step(bank_stmt);
    sqlite3_finalize(bank_stmt);
    if (ret_code != SQLITE_ROW)
    {
        return commons::Result::NotFound;
    }

    // Ensure member exists
    const char* member_check_sql = "SELECT 1 FROM MemberInfo WHERE Member_ID = ?;";
    sqlite3_stmt* member_stmt = nullptr;
    ret_code = sqlite3_prepare_v2(db_handle, member_check_sql, -1, &member_stmt, nullptr);
    if (ret_code != SQLITE_OK)
    {
        if (member_stmt) sqlite3_finalize(member_stmt);
        return commons::Result::DbError;
    }

    sqlite3_bind_int64(member_stmt, 1, static_cast<sqlite3_int64>(member_id));
    ret_code = sqlite3_step(member_stmt);
    sqlite3_finalize(member_stmt);
    if (ret_code != SQLITE_ROW)
    {
        return commons::Result::NotFound;
    }

    const char* insert_sql = "INSERT INTO BankAccounts (Bank_ID, Member_ID, Account_Number, Opening_Balance, Closing_Balance) VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt* insert_stmt = nullptr;
    ret_code = sqlite3_prepare_v2(db_handle, insert_sql, -1, &insert_stmt, nullptr);
    if (ret_code != SQLITE_OK)
    {
        if (insert_stmt) sqlite3_finalize(insert_stmt);
        return commons::Result::DbError;
    }

    sqlite3_bind_int64(insert_stmt, 1, static_cast<sqlite3_int64>(bank_id));
    sqlite3_bind_int64(insert_stmt, 2, static_cast<sqlite3_int64>(member_id));
    sqlite3_bind_text(insert_stmt, 3, account_number.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(insert_stmt, 4, static_cast<sqlite3_int64>(opening_paise));
    sqlite3_bind_int64(insert_stmt, 5, static_cast<sqlite3_int64>(closing_paise));

    ret_code = sqlite3_step(insert_stmt);
    if (ret_code != SQLITE_DONE)
    {
        sqlite3_finalize(insert_stmt);
        return commons::Result::DbError;
    }

    sqlite3_finalize(insert_stmt);

    if (out_id)
    {
        *out_id = static_cast<uint64_t>(sqlite3_last_insert_rowid(db_handle));
    }

    return commons::Result::Ok;
}

bool StorageManager::saveBankAccount(uint64_t bank_id,
                                    uint64_t member_id,
                                    const std::string &account_number,
                                    long long opening_paise,
                                    long long closing_paise)
{
    return saveBankAccountEx(bank_id, member_id, account_number, opening_paise, closing_paise, nullptr) == commons::Result::Ok;
}

/**
 * @brief Delete member data.
 * 
 * @param member_id ID of the member to delete.
 * @return commons::Result 
 */
commons::Result StorageManager::deleteMemberDataEx(const uint64_t& member_id)
{
    if (!connected) 
    {
        if (!initializeDatabase(""))
        {
            return commons::Result::DbError;
        } 
    }

    const char* sql = "DELETE FROM MemberInfo WHERE Member_ID = ?;";
    sqlite3_stmt* stmt = nullptr;
    int ret_code = sqlite3_prepare_v2(db_handle, sql, -1, &stmt, nullptr);

    if (ret_code != SQLITE_OK)
    {
        return commons::Result::DbError;
    } 

    sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(member_id));
    ret_code = sqlite3_step(stmt);

    if (ret_code != SQLITE_DONE) 
    {
        sqlite3_finalize(stmt);
        return commons::Result::DbError;
    }

    int changes = sqlite3_changes(db_handle);
    sqlite3_finalize(stmt);

    return (changes > 0) ? commons::Result::Ok : commons::Result::NotFound;
}

/**
 * @brief Delete family data.
 * 
 * @param family_id ID of the family to delete.
 * @return commons::Result 
 */
commons::Result StorageManager::deleteFamilyDataEx(const uint64_t& family_id)
{
    if (!connected) 
    {
        if (!initializeDatabase(""))
        {
            return commons::Result::DbError;
        }
    }

    const char* sql = "DELETE FROM FamilyInfo WHERE Family_ID = ?;";
    sqlite3_stmt* stmt = nullptr;
    int ret_code = sqlite3_prepare_v2(db_handle, sql, -1, &stmt, nullptr);

    if (ret_code != SQLITE_OK)
    {
        return commons::Result::DbError;
    }

    sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(family_id));
    ret_code = sqlite3_step(stmt);

    if (ret_code != SQLITE_DONE) 
    {
        sqlite3_finalize(stmt);
        return commons::Result::DbError;
    }

    int changes = sqlite3_changes(db_handle);
    sqlite3_finalize(stmt);

    return (changes > 0) ? commons::Result::Ok : commons::Result::NotFound;
}

/**
 * @brief Update family data.
 * 
 * @param family_id ID of the family to update.
 * @param new_name Name to update to.
 * @return commons::Result 
 */
commons::Result StorageManager::updateFamilyDataEx(const uint64_t& family_id, const std::string& new_name)
{
    if (new_name.empty())
    {
        return commons::Result::InvalidInput;
    }
    
    if (!connected) 
    {
        if (!initializeDatabase(""))
        {
            return commons::Result::DbError;
        } 
    }

    const char* sql = "UPDATE FamilyInfo SET Family_Name = ? WHERE Family_ID = ?;";
    sqlite3_stmt* stmt = nullptr;
    int ret_code = sqlite3_prepare_v2(db_handle, sql, -1, &stmt, nullptr);

    if (ret_code != SQLITE_OK)
    {
        return commons::Result::DbError;
    } 

    sqlite3_bind_text(stmt, 1, new_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, static_cast<sqlite3_int64>(family_id));
    ret_code = sqlite3_step(stmt);

    if (ret_code != SQLITE_DONE) 
    {
        sqlite3_finalize(stmt);
        return commons::Result::DbError;
    }

    int changes = sqlite3_changes(db_handle);
    sqlite3_finalize(stmt);

    return (changes > 0) ? commons::Result::Ok : commons::Result::NotFound;
}

/**
 * @brief Update member data.
 * 
 * @param member_id ID of the member to update.
 * @param new_name Name to update to.
 * @param new_nickname Nickname to update to.
 * @return commons::Result 
 */
commons::Result StorageManager::updateMemberDataEx(const uint64_t& member_id, const std::string& new_name, const std::string& new_nickname)
{
    // Check if we have at least one field to update
    bool update_name = !new_name.empty();
    bool update_nickname = !new_nickname.empty();
    if (!update_name && !update_nickname)
    {
        return commons::Result::InvalidInput;
    }

    if (!connected) 
    {
        if (!initializeDatabase(""))
        {
            return commons::Result::DbError;
        }
    }

    // Build dynamic SQL based on what needs to be updated
    std::string sql = "UPDATE MemberInfo SET ";
    if (update_name)
    {
        sql += "Member_Name = ?";
        if (update_nickname)
        {
            sql += ", ";
        }
    }
    if (update_nickname)
    {
        sql += "Member_Nick_Name = ?";
    }
    sql += " WHERE Member_ID = ?;";

    sqlite3_stmt* stmt = nullptr;
    int ret_code = sqlite3_prepare_v2(db_handle, sql.c_str(), -1, &stmt, nullptr);

    if (ret_code != SQLITE_OK)
    {
        return commons::Result::DbError;
    } 

    int param_index = 1;
    if (update_name)
    {
        sqlite3_bind_text(stmt, param_index++, new_name.c_str(), -1, SQLITE_TRANSIENT);
    }
    if (update_nickname)
    {
        sqlite3_bind_text(stmt, param_index++, new_nickname.c_str(), -1, SQLITE_TRANSIENT);
    }
    sqlite3_bind_int64(stmt, param_index, static_cast<sqlite3_int64>(member_id));
    ret_code = sqlite3_step(stmt);

    if (ret_code != SQLITE_DONE) 
    {
        sqlite3_finalize(stmt);
        return commons::Result::DbError;
    }

    int changes = sqlite3_changes(db_handle);
    sqlite3_finalize(stmt);

    return (changes > 0) ? commons::Result::Ok : commons::Result::NotFound;
}

/**
 * @brief Connect to the database.
 * 
 * @param connectionString SQLite database file path.
 * @return true if connection was successful.
 * @return false if connection failed.
 */
bool StorageManager::connect(const std::string& connectionString) 
{
    // If already connected and same path, return true. If different path, close and reopen.
    if (connected && db_handle) 
    {
        // Note: no stored path is kept; caller should manage connections if using multiple DBs.
        return true;
    }

    // Open SQLite DB (read/write, create if missing)
    int ret_code = sqlite3_open_v2(connectionString.c_str(), &db_handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
    
    if (ret_code != SQLITE_OK) 
    {
        std::cerr << "Failed to open SQLite DB '" << connectionString << "': " << (db_handle ? sqlite3_errmsg(db_handle) : "(no handle)") << std::endl;
        if (db_handle) 
        {
            sqlite3_close(db_handle);
            db_handle = nullptr;
        }

        connected = false;
        return false;
    }

    // Enable foreign keys
    char* errmsg = nullptr;
    ret_code = sqlite3_exec(db_handle, "PRAGMA foreign_keys = ON;", nullptr, nullptr, &errmsg);
    
    if (ret_code != SQLITE_OK) 
    {
        std::cerr << "Warning: failed to enable foreign keys: " << (errmsg ? errmsg : "") << std::endl;
        if (errmsg) sqlite3_free(errmsg);
        // Not fatal; proceed
    }

    connected = true;
    return true;
}

/**
 * @brief Disconnect from the database.
 * 
 */
void StorageManager::disconnect() 
{
    if (db_handle) 
    {
        sqlite3_close(db_handle);
        db_handle = nullptr;
    }

    connected = false;
}

/**
 * @brief List all families in the database.
 * 
 * @return std::vector<Family> Vector of families with IDs
 */
std::vector<Family> StorageManager::listFamilies()
{
    std::vector<Family> families;
    if (!connected)
    {
        if (!initializeDatabase(""))
        {
            return families;
        }
    }

    const char* sql = "SELECT Family_ID, Family_Name FROM FamilyInfo ORDER BY Family_ID;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_handle, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        return families;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        uint64_t id = static_cast<uint64_t>(sqlite3_column_int64(stmt, 0));
        const unsigned char* name = sqlite3_column_text(stmt, 1);
        std::string name_str = name ? reinterpret_cast<const char*>(name) : std::string();
        families.emplace_back(id, name_str);
    }
    sqlite3_finalize(stmt);
    return families;
}

/**
 * @brief List all members of a specific family.
 * 
 * @param family_id ID of the family whose members to list
 * @return std::vector<Member> Vector of members with IDs
 */
std::vector<Member> StorageManager::listMembersOfFamily(uint64_t family_id)
{
    std::vector<Member> members;
    if (!connected)
    {
        if (!initializeDatabase(""))
        {
            return members;
        }
    }

    const char* sql = "SELECT Member_ID, Member_Name, Member_Nick_Name FROM MemberInfo WHERE Family_ID = ? ORDER BY Member_ID;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_handle, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        return members;
    }

    sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(family_id));
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        uint64_t id = static_cast<uint64_t>(sqlite3_column_int64(stmt, 0));
        const unsigned char* name = sqlite3_column_text(stmt, 1);
        const unsigned char* nick = sqlite3_column_text(stmt, 2);
        std::string name_str = name ? reinterpret_cast<const char*>(name) : std::string();
        std::string nick_str = nick ? reinterpret_cast<const char*>(nick) : std::string();
        members.emplace_back(id, name_str, nick_str);
    }
    sqlite3_finalize(stmt);
    return members;
}

uint64_t StorageManager::getMemberCount(const uint64_t family_id, bool* out_ok)
{
    if (!connected)
    {
        if (!initializeDatabase(""))
        {
            if (out_ok) *out_ok = false;
            return 0;
        }
    }

    const char* sql = "SELECT COUNT(1) FROM MemberInfo WHERE Family_ID = ?;";
    sqlite3_stmt* stmt = nullptr;
    int ret_code = sqlite3_prepare_v2(db_handle, sql, -1, &stmt, nullptr);
    if (ret_code != SQLITE_OK)
    {
        if (stmt) sqlite3_finalize(stmt);
        if (out_ok) *out_ok = false;
        return 0;
    }

    sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(family_id));
    ret_code = sqlite3_step(stmt);
    if (ret_code != SQLITE_ROW)
    {
        sqlite3_finalize(stmt);
        if (out_ok) *out_ok = false;
        return 0;
    }

    sqlite3_int64 cnt = sqlite3_column_int64(stmt, 0);
    sqlite3_finalize(stmt);

    if (cnt < 0)
    {
        if (out_ok) *out_ok = false;
        return 0;
    }

    if (out_ok) *out_ok = true;
    return static_cast<uint64_t>(cnt);
}

commons::Result StorageManager::getBankIdByName(const std::string &bank_name, uint64_t* out_bank_id)
{
    if (!connected)
    {
        if (!initializeDatabase(""))
        {
            return commons::Result::DbError;
        }
    }

    const char* sql = "SELECT Bank_ID FROM BankList WHERE lower(Bank_Name) = lower(?) LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;
    int ret_code = sqlite3_prepare_v2(db_handle, sql, -1, &stmt, nullptr);
    if (ret_code != SQLITE_OK)
    {
        if (stmt) sqlite3_finalize(stmt);
        return commons::Result::DbError;
    }

    sqlite3_bind_text(stmt, 1, bank_name.c_str(), -1, SQLITE_TRANSIENT);
    ret_code = sqlite3_step(stmt);
    if (ret_code != SQLITE_ROW)
    {
        sqlite3_finalize(stmt);
        return commons::Result::NotFound;
    }

    sqlite3_int64 id = sqlite3_column_int64(stmt, 0);
    sqlite3_finalize(stmt);
    if (out_bank_id) *out_bank_id = static_cast<uint64_t>(id);
    return commons::Result::Ok;
}

commons::Result StorageManager::getBankNameById(const uint64_t bank_id, std::string* out_name)
{
    if (!out_name)
    {
        return commons::Result::InvalidInput;
    }

    if (!connected)
    {
        if (!initializeDatabase(""))
        {
            return commons::Result::DbError;
        }
    }

    const char* sql = "SELECT Bank_Name FROM BankList WHERE Bank_ID = ? LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;
    int ret_code = sqlite3_prepare_v2(db_handle, sql, -1, &stmt, nullptr);
    if (ret_code != SQLITE_OK)
    {
        if (stmt) sqlite3_finalize(stmt);
        return commons::Result::DbError;
    }

    sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(bank_id));
    ret_code = sqlite3_step(stmt);
    if (ret_code != SQLITE_ROW)
    {
        sqlite3_finalize(stmt);
        return commons::Result::NotFound;
    }

    const unsigned char* txt = sqlite3_column_text(stmt, 0);
    *out_name = txt ? reinterpret_cast<const char*>(txt) : std::string();
    sqlite3_finalize(stmt);
    return commons::Result::Ok;
}

commons::Result StorageManager::getBankAccountById(const uint64_t bank_account_id, BankAccount* out_row)
{
    if (!out_row)
    {
        return commons::Result::InvalidInput;
    }

    if (!connected)
    {
        if (!initializeDatabase(""))
        {
            return commons::Result::DbError;
        }
    }

    const char* sql = "SELECT BankAccount_ID, Bank_ID, Member_ID, Account_Number, Opening_Balance, Closing_Balance FROM BankAccounts WHERE BankAccount_ID = ? LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;
    int ret_code = sqlite3_prepare_v2(db_handle, sql, -1, &stmt, nullptr);
    if (ret_code != SQLITE_OK)
    {
        if (stmt) sqlite3_finalize(stmt);
        return commons::Result::DbError;
    }

    sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(bank_account_id));
    ret_code = sqlite3_step(stmt);
    if (ret_code != SQLITE_ROW)
    {
        sqlite3_finalize(stmt);
        return commons::Result::NotFound;
    }

    out_row->setId(static_cast<uint64_t>(sqlite3_column_int64(stmt, 0)));
    out_row->setBankId(static_cast<uint64_t>(sqlite3_column_int64(stmt, 1)));
    out_row->setMemberId(static_cast<uint64_t>(sqlite3_column_int64(stmt, 2)));
    const unsigned char* acct = sqlite3_column_text(stmt, 3);
    out_row->setAccountNumber(acct ? reinterpret_cast<const char*>(acct) : std::string());
    out_row->setOpeningBalancePaise(static_cast<long long>(sqlite3_column_int64(stmt, 4)));
    out_row->setClosingBalancePaise(static_cast<long long>(sqlite3_column_int64(stmt, 5)));

    sqlite3_finalize(stmt);
    return commons::Result::Ok;
}

std::vector<BankAccount> StorageManager::listBankAccountsOfMember(const uint64_t member_id)
{
    std::vector<BankAccount> rows;

    if (!connected)
    {
        if (!initializeDatabase(""))
        {
            return rows;
        }
    }

    const char* sql = "SELECT BankAccount_ID, Bank_ID, Member_ID, Account_Number, Opening_Balance, Closing_Balance FROM BankAccounts WHERE Member_ID = ? ORDER BY BankAccount_ID;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_handle, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        return rows;
    }

    sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(member_id));

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
    BankAccount r;
    r.setId(static_cast<uint64_t>(sqlite3_column_int64(stmt, 0)));
    r.setBankId(static_cast<uint64_t>(sqlite3_column_int64(stmt, 1)));
    r.setMemberId(static_cast<uint64_t>(sqlite3_column_int64(stmt, 2)));
    const unsigned char* acct = sqlite3_column_text(stmt, 3);
    r.setAccountNumber(acct ? reinterpret_cast<const char*>(acct) : std::string());
    r.setOpeningBalancePaise(static_cast<long long>(sqlite3_column_int64(stmt, 4)));
    r.setClosingBalancePaise(static_cast<long long>(sqlite3_column_int64(stmt, 5)));
    rows.push_back(r);
    }

    sqlite3_finalize(stmt);
    return rows;
}
