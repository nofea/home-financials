#include "storage_manager.hpp"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include <sqlite3.h>

// Constructor
StorageManager::StorageManager() 
{
    // TODO: Initialize storage manager
}

// Destructor
StorageManager::~StorageManager() 
{
    // Ensure DB handle is closed on destruction
    disconnect();
}

bool StorageManager::initializeDatabase(const std::string& dbPath) 
{
    // Determine database path. If caller provided a path (non-empty and not the
    // placeholder), use it. Otherwise, try to infer project root and use
    // project_root/homefinancials.db
    std::string chosenPath = dbPath;
    if (chosenPath.empty() || chosenPath == "path/to/database") {
        namespace fs = std::filesystem;
        fs::path projectRoot;
        try {
            fs::path exe = fs::read_symlink("/proc/self/exe");
            projectRoot = exe.parent_path().parent_path();
            if (!fs::exists(projectRoot)) {
                projectRoot = exe.parent_path();
            }
        } catch (...) {
            projectRoot = fs::current_path();
        }
        fs::path dbPathFs = projectRoot / "homefinancials.db";
        chosenPath = dbPathFs.string();
    }

    // Ensure tables exist at the chosen path
    runDbInitScript(chosenPath);

    // Try to connect to the database
    return connect(chosenPath);
}

// Run the Python DB init script from the project root and ensure the database
// file is created there. This mirrors the previous free function but lives
// on the StorageManager class. We consider the project root to be either
// the executable parent->parent (common when binary is in build/bin) or the
// current working directory. We pass the intended DB path to the Python
// script so it will create the DB at project_root/homefinancials.db.
void StorageManager::runDbInitScript(const std::string& dbPathStr)
{
    namespace fs = std::filesystem;
    fs::path dbPath = fs::path(dbPathStr);
    fs::path projectRoot = dbPath.parent_path();

    std::cout << "Initializing SQLite DB at: " << dbPath << std::endl;

    // Create parent directories if needed
    try {
        if (!projectRoot.empty() && !fs::exists(projectRoot)) {
            fs::create_directories(projectRoot);
        }
    } catch (const std::exception &e) {
        std::cerr << "Failed to create DB parent directories: " << e.what() << std::endl;
    }

    // SQL statements to create only the required tables: FamilyInfo and MemberInfo
    const std::vector<std::pair<const char*, const char*>> table_ddls = {
        {"FamilyInfo", R"(
        CREATE TABLE IF NOT EXISTS FamilyInfo (
            Family_ID INTEGER PRIMARY KEY AUTOINCREMENT,
            Family_Name TEXT NOT NULL
        );
    )"},
        {"MemberInfo", R"(
        CREATE TABLE IF NOT EXISTS MemberInfo (
            Member_ID INTEGER PRIMARY KEY AUTOINCREMENT,
            Family_ID INTEGER NOT NULL,
            Member_Name TEXT NOT NULL,
            Member_Nick_Name TEXT,
            FOREIGN KEY(Family_ID) REFERENCES FamilyInfo(Family_ID) ON DELETE CASCADE
        );
    )"}
    };

    sqlite3* db = nullptr;
    int rc = sqlite3_open(dbPath.string().c_str(), &db);
    if (rc != SQLITE_OK) {
        std::cerr << "Cannot open SQLite DB: " << (db ? sqlite3_errmsg(db) : "(no handle)") << std::endl;
        if (db) sqlite3_close(db);
        return;
    }

    // Enable foreign key enforcement
    char* errmsg = nullptr;
    rc = sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to enable foreign keys: " << (errmsg ? errmsg : "") << std::endl;
        sqlite3_free(errmsg);
    }

    for (const auto &p : table_ddls) {
        rc = sqlite3_exec(db, p.second, nullptr, nullptr, &errmsg);
        if (rc != SQLITE_OK) {
            std::cerr << "Error creating table '" << p.first << "': " << (errmsg ? errmsg : "") << std::endl;
            sqlite3_free(errmsg);
        } else {
            std::cout << "Checked/created table: " << p.first << std::endl;
        }
    }

    sqlite3_close(db);
}

bool StorageManager::saveMemberData(const Member& member, const uint64_t family_id) 
{
    // Ensure DB is ready
    if (!connected_) {
        if (!initializeDatabase("")) return false;
    }

    // First verify the family exists
    const char* check_sql = "SELECT 1 FROM FamilyInfo WHERE Family_ID = ?;";
    sqlite3_stmt* check_stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_handle_, check_sql, -1, &check_stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare family check statement: " << sqlite3_errmsg(db_handle_) << std::endl;
        return false;
    }
    sqlite3_bind_int64(check_stmt, 1, static_cast<sqlite3_int64>(family_id));
    rc = sqlite3_step(check_stmt);
    sqlite3_finalize(check_stmt);
    if (rc != SQLITE_ROW) {
        std::cerr << "Cannot add member: Family with ID " << family_id << " does not exist" << std::endl;
        return false;
    }

    const char* sql = "INSERT INTO MemberInfo (Family_ID, Member_Name, Member_Nick_Name) VALUES (?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    rc = sqlite3_prepare_v2(db_handle_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare insert member statement: " << sqlite3_errmsg(db_handle_) << std::endl;
        return false;
    }

    rc = sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(family_id));
    rc |= sqlite3_bind_text(stmt, 2, member.getName().c_str(), -1, SQLITE_TRANSIENT);
    rc |= sqlite3_bind_text(stmt, 3, member.getNickname().c_str(), -1, SQLITE_TRANSIENT);

    if (rc != SQLITE_OK) {
        std::cerr << "Failed to bind insert member values: " << sqlite3_errmsg(db_handle_) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute insert member: " << sqlite3_errmsg(db_handle_) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

bool StorageManager::saveFamilyData(const Family& family) 
{
    if (!connected_) {
        if (!initializeDatabase("")) return false;
    }

    const char* sql = "INSERT INTO FamilyInfo (Family_Name) VALUES (?);";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_handle_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare insert family statement: " << sqlite3_errmsg(db_handle_) << std::endl;
        return false;
    }

    rc = sqlite3_bind_text(stmt, 1, family.getName().c_str(), -1, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to bind insert family values: " << sqlite3_errmsg(db_handle_) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute insert family: " << sqlite3_errmsg(db_handle_) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }

    // Optionally insert members if provided
    sqlite3_finalize(stmt);
    sqlite3_int64 family_id = sqlite3_last_insert_rowid(db_handle_);

    auto members = family.getMembers();
    for (const auto &m : members) {
        const char* msql = "INSERT INTO MemberInfo (Family_ID, Member_Name, Member_Nick_Name) VALUES (?, ?, ?);";
        sqlite3_stmt* mstmt = nullptr;
        rc = sqlite3_prepare_v2(db_handle_, msql, -1, &mstmt, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare insert member for family: " << sqlite3_errmsg(db_handle_) << std::endl;
            continue;
        }
        sqlite3_bind_int64(mstmt, 1, family_id);
        sqlite3_bind_text(mstmt, 2, m.getName().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(mstmt, 3, m.getNickname().c_str(), -1, SQLITE_TRANSIENT);
        rc = sqlite3_step(mstmt);
        if (rc != SQLITE_DONE) {
            std::cerr << "Failed to insert member for family: " << sqlite3_errmsg(db_handle_) << std::endl;
        }
        sqlite3_finalize(mstmt);
    }

    return true;
}

Member* StorageManager::getMemberData(const uint64_t& member_id) 
{
    if (!connected_) {
        if (!initializeDatabase("")) return nullptr;
    }

    const char* sql = "SELECT Member_ID, Family_ID, Member_Name, Member_Nick_Name FROM MemberInfo WHERE Member_ID = ?;";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_handle_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare select member: " << sqlite3_errmsg(db_handle_) << std::endl;
        return nullptr;
    }
    sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(member_id));
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
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

Family* StorageManager::getFamilyData(const uint64_t& family_id) 
{
    if (!connected_) {
        if (!initializeDatabase("")) return nullptr;
    }

    const char* fsql = "SELECT Family_ID, Family_Name FROM FamilyInfo WHERE Family_ID = ?;";
    sqlite3_stmt* fstmt = nullptr;
    int rc = sqlite3_prepare_v2(db_handle_, fsql, -1, &fstmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare select family: " << sqlite3_errmsg(db_handle_) << std::endl;
        return nullptr;
    }
    sqlite3_bind_int64(fstmt, 1, static_cast<sqlite3_int64>(family_id));
    rc = sqlite3_step(fstmt);
    if (rc != SQLITE_ROW) {
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
    rc = sqlite3_prepare_v2(db_handle_, msql, -1, &mstmt, nullptr);
    if (rc == SQLITE_OK) {
        sqlite3_bind_int64(mstmt, 1, static_cast<sqlite3_int64>(family_id));
        while ((rc = sqlite3_step(mstmt)) == SQLITE_ROW) {
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

bool StorageManager::deleteMemberData(const uint64_t& member_id) 
{
    if (!connected_) {
        if (!initializeDatabase("")) return false;
    }
    const char* sql = "DELETE FROM MemberInfo WHERE Member_ID = ?;";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_handle_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare delete member: " << sqlite3_errmsg(db_handle_) << std::endl;
        return false;
    }
    sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(member_id));
    rc = sqlite3_step(stmt);
    bool success = (rc == SQLITE_DONE && sqlite3_changes(db_handle_) > 0);
    sqlite3_finalize(stmt);
    return success;
}

bool StorageManager::deleteFamilyData(const uint64_t& family_id) 
{
    if (!connected_) {
        if (!initializeDatabase("")) return false;
    }
    const char* sql = "DELETE FROM FamilyInfo WHERE Family_ID = ?;";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_handle_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare delete family: " << sqlite3_errmsg(db_handle_) << std::endl;
        return false;
    }
    sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(family_id));
    rc = sqlite3_step(stmt);
    bool success = (rc == SQLITE_DONE && sqlite3_changes(db_handle_) > 0);
    sqlite3_finalize(stmt);
    return success;
}

// Connect to the database
bool StorageManager::connect(const std::string& connectionString) 
{
    // If already connected and same path, return true. If different path, close and reopen.
    if (connected_ && db_handle_) {
        // Note: no stored path is kept; caller should manage connections if using multiple DBs.
        return true;
    }

    // Open SQLite DB (read/write, create if missing)
    int rc = sqlite3_open_v2(connectionString.c_str(), &db_handle_, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to open SQLite DB '" << connectionString << "': " << (db_handle_ ? sqlite3_errmsg(db_handle_) : "(no handle)") << std::endl;
        if (db_handle_) {
            sqlite3_close(db_handle_);
            db_handle_ = nullptr;
        }
        connected_ = false;
        return false;
    }

    // Enable foreign keys
    char* errmsg = nullptr;
    rc = sqlite3_exec(db_handle_, "PRAGMA foreign_keys = ON;", nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Warning: failed to enable foreign keys: " << (errmsg ? errmsg : "") << std::endl;
        if (errmsg) sqlite3_free(errmsg);
        // Not fatal; proceed
    }

    connected_ = true;
    return true;
}

// Disconnect from the database
void StorageManager::disconnect() 
{
    if (db_handle_) {
        sqlite3_close(db_handle_);
        db_handle_ = nullptr;
    }
    connected_ = false;
}
