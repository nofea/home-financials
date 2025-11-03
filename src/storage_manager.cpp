#include "storage_manager.hpp"
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
    // TODO: Initialize storage manager
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
    return saveMemberDataEx(member, family_id, nullptr) == Result::Ok;
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
    return saveFamilyDataEx(family, nullptr) == Result::Ok;
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
 * @return true 
 * @return false 
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

bool StorageManager::updateMemberData(const uint64_t& member_id, const std::string& new_name, const std::string& new_nickname)
{
    if (!connected) {
        if (!initializeDatabase("")) {
            return false;
        }
    }

    const char* sql = "UPDATE MemberInfo SET Member_Name = ?, Member_Nick_Name = ? WHERE Member_ID = ?;";
    sqlite3_stmt* stmt = nullptr;
    int ret_code = sqlite3_prepare_v2(db_handle, sql, -1, &stmt, nullptr);
    if (ret_code != SQLITE_OK) {
        std::cerr << "Failed to prepare update member: " << sqlite3_errmsg(db_handle) << std::endl;
        return false;
    }
    sqlite3_bind_text(stmt, 1, new_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, new_nickname.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, static_cast<sqlite3_int64>(member_id));
    ret_code = sqlite3_step(stmt);
    bool success = (ret_code == SQLITE_DONE && sqlite3_changes(db_handle) > 0);
    sqlite3_finalize(stmt);
    return success;
}

// Extended APIs returning Result codes

StorageManager::Result StorageManager::saveFamilyDataEx(const Family& family, uint64_t* out_family_id)
{
    if (family.getName().empty()) {
        return Result::InvalidInput;
    }

    if (!connected) {
        if (!initializeDatabase("")) return Result::DbError;
    }

    const char* sql = "INSERT INTO FamilyInfo (Family_Name) VALUES (?);";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_handle, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return Result::DbError;
    }

    rc = sqlite3_bind_text(stmt, 1, family.getName().c_str(), -1, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) {
        sqlite3_finalize(stmt);
        return Result::DbError;
    }

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return Result::DbError;
    }

    sqlite3_finalize(stmt);
    sqlite3_int64 family_id = sqlite3_last_insert_rowid(db_handle);

    // Insert members if any
    auto members = family.getMembers();
    for (const auto &m : members) {
        const char* msql = "INSERT INTO MemberInfo (Family_ID, Member_Name, Member_Nick_Name) VALUES (?, ?, ?);";
        sqlite3_stmt* mstmt = nullptr;
        rc = sqlite3_prepare_v2(db_handle, msql, -1, &mstmt, nullptr);
        if (rc != SQLITE_OK) {
            // continue inserting others but mark DB error
            sqlite3_finalize(mstmt);
            continue;
        }
        sqlite3_bind_int64(mstmt, 1, family_id);
        sqlite3_bind_text(mstmt, 2, m.getName().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(mstmt, 3, m.getNickname().c_str(), -1, SQLITE_TRANSIENT);
        rc = sqlite3_step(mstmt);
        sqlite3_finalize(mstmt);
    }

    if (out_family_id) *out_family_id = static_cast<uint64_t>(family_id);
    return Result::Ok;
}

StorageManager::Result StorageManager::saveMemberDataEx(const Member& member, const uint64_t family_id, uint64_t* out_member_id)
{
    if (member.getName().empty()) return Result::InvalidInput;
    if (family_id == 0) return Result::InvalidInput;

    if (!connected) {
        if (!initializeDatabase("")) return Result::DbError;
    }

    // Check family exists
    const char* check_sql = "SELECT 1 FROM FamilyInfo WHERE Family_ID = ?;";
    sqlite3_stmt* check_stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_handle, check_sql, -1, &check_stmt, nullptr);
    if (rc != SQLITE_OK) return Result::DbError;
    sqlite3_bind_int64(check_stmt, 1, static_cast<sqlite3_int64>(family_id));
    rc = sqlite3_step(check_stmt);
    sqlite3_finalize(check_stmt);
    if (rc != SQLITE_ROW) return Result::NotFound;

    const char* sql = "INSERT INTO MemberInfo (Family_ID, Member_Name, Member_Nick_Name) VALUES (?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    rc = sqlite3_prepare_v2(db_handle, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return Result::DbError;

    sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(family_id));
    sqlite3_bind_text(stmt, 2, member.getName().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, member.getNickname().c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return Result::DbError;
    }

    sqlite3_finalize(stmt);
    if (out_member_id) *out_member_id = static_cast<uint64_t>(sqlite3_last_insert_rowid(db_handle));
    return Result::Ok;
}

StorageManager::Result StorageManager::deleteMemberDataEx(const uint64_t& member_id)
{
    if (!connected) {
        if (!initializeDatabase("")) return Result::DbError;
    }

    const char* sql = "DELETE FROM MemberInfo WHERE Member_ID = ?;";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_handle, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return Result::DbError;

    sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(member_id));
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return Result::DbError;
    }

    int changes = sqlite3_changes(db_handle);
    sqlite3_finalize(stmt);
    return (changes > 0) ? Result::Ok : Result::NotFound;
}

StorageManager::Result StorageManager::deleteFamilyDataEx(const uint64_t& family_id)
{
    if (!connected) {
        if (!initializeDatabase("")) return Result::DbError;
    }

    const char* sql = "DELETE FROM FamilyInfo WHERE Family_ID = ?;";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_handle, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return Result::DbError;

    sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(family_id));
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return Result::DbError;
    }

    int changes = sqlite3_changes(db_handle);
    sqlite3_finalize(stmt);
    return (changes > 0) ? Result::Ok : Result::NotFound;
}

StorageManager::Result StorageManager::updateFamilyDataEx(const uint64_t& family_id, const std::string& new_name)
{
    if (new_name.empty()) return Result::InvalidInput;
    if (!connected) {
        if (!initializeDatabase("")) return Result::DbError;
    }

    const char* sql = "UPDATE FamilyInfo SET Family_Name = ? WHERE Family_ID = ?;";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_handle, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return Result::DbError;

    sqlite3_bind_text(stmt, 1, new_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, static_cast<sqlite3_int64>(family_id));
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return Result::DbError;
    }

    int changes = sqlite3_changes(db_handle);
    sqlite3_finalize(stmt);
    return (changes > 0) ? Result::Ok : Result::NotFound;
}

StorageManager::Result StorageManager::updateMemberDataEx(const uint64_t& member_id, const std::string& new_name, const std::string& new_nickname)
{
    if (new_name.empty()) return Result::InvalidInput;
    if (!connected) {
        if (!initializeDatabase("")) return Result::DbError;
    }

    const char* sql = "UPDATE MemberInfo SET Member_Name = ?, Member_Nick_Name = ? WHERE Member_ID = ?;";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_handle, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return Result::DbError;

    sqlite3_bind_text(stmt, 1, new_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, new_nickname.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, static_cast<sqlite3_int64>(member_id));
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return Result::DbError;
    }

    int changes = sqlite3_changes(db_handle);
    sqlite3_finalize(stmt);
    return (changes > 0) ? Result::Ok : Result::NotFound;
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
