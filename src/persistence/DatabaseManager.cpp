#include "DatabaseManager.h"
#include "../utils/Logger.h"
#include <filesystem>
#include <sstream>
#include <algorithm>

namespace KitchenCAD {
namespace Persistence {

DatabaseManager::DatabaseManager() 
    : db_(nullptr), isOpen_(false) {
    initializeMigrations();
}

DatabaseManager::DatabaseManager(const std::string& dbPath) 
    : db_(nullptr), dbPath_(dbPath), isOpen_(false) {
    initializeMigrations();
    open(dbPath);
}

DatabaseManager::~DatabaseManager() {
    close();
}

bool DatabaseManager::open(const std::string& dbPath) {
    if (isOpen_) {
        close();
    }
    
    dbPath_ = dbPath;
    
    // Create directory if it doesn't exist
    std::filesystem::path path(dbPath);
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path());
    }
    
    int result = sqlite3_open(dbPath.c_str(), &db_);
    if (result != SQLITE_OK) {
        LOG_ERROR("Failed to open database: " + std::string(sqlite3_errmsg(db_)));
        sqlite3_close(db_);
        db_ = nullptr;
        return false;
    }
    
    isOpen_ = true;
    
    // Enable foreign keys
    execute("PRAGMA foreign_keys = ON");
    
    // Set journal mode to WAL for better concurrency
    execute("PRAGMA journal_mode = WAL");
    
    // Run migrations
    if (!runMigrations()) {
        LOG_ERROR("Failed to run database migrations");
        close();
        return false;
    }
    
    LOG_INFO("Database opened successfully: " + dbPath);
    return true;
}

void DatabaseManager::close() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
    isOpen_ = false;
}

bool DatabaseManager::beginTransaction() {
    return execute("BEGIN TRANSACTION");
}

bool DatabaseManager::commitTransaction() {
    return execute("COMMIT");
}

bool DatabaseManager::rollbackTransaction() {
    return execute("ROLLBACK");
}

bool DatabaseManager::execute(const std::string& sql) {
    if (!isOpen_) {
        LOG_ERROR("Database is not open");
        return false;
    }
    
    char* errorMsg = nullptr;
    int result = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errorMsg);
    
    if (result != SQLITE_OK) {
        std::string error = errorMsg ? errorMsg : "Unknown error";
        LOG_ERROR("SQL execution failed: " + error + " (SQL: " + sql + ")");
        if (errorMsg) {
            sqlite3_free(errorMsg);
        }
        return false;
    }
    
    return true;
}

bool DatabaseManager::execute(const std::string& sql, const std::vector<std::string>& params) {
    auto stmt = prepare(sql);
    if (!stmt || !stmt->isValid()) {
        return false;
    }
    
    for (size_t i = 0; i < params.size(); ++i) {
        if (!stmt->bindText(static_cast<int>(i + 1), params[i])) {
            return false;
        }
    }
    
    return stmt->step();
}

std::unique_ptr<DatabaseManager::PreparedStatement> DatabaseManager::prepare(const std::string& sql) {
    if (!isOpen_) {
        LOG_ERROR("Database is not open");
        return nullptr;
    }
    
    return std::make_unique<PreparedStatement>(db_, sql);
}

void DatabaseManager::addMigration(const Migration& migration) {
    migrations_.push_back(migration);
    std::sort(migrations_.begin(), migrations_.end(), 
              [](const Migration& a, const Migration& b) {
                  return a.version < b.version;
              });
}

bool DatabaseManager::runMigrations() {
    if (!createMigrationTable()) {
        return false;
    }
    
    int currentVersion = getCurrentVersion();
    
    for (const auto& migration : migrations_) {
        if (migration.version <= currentVersion) {
            continue;
        }
        
        LOG_INFO("Running migration " + std::to_string(migration.version) + ": " + migration.description);
        
        Transaction transaction(*this);
        
        for (const auto& sql : migration.sqlStatements) {
            if (!execute(sql)) {
                LOG_ERROR("Migration " + std::to_string(migration.version) + " failed");
                return false;
            }
        }
        
        if (!setVersion(migration.version)) {
            LOG_ERROR("Failed to update version for migration " + std::to_string(migration.version));
            return false;
        }
        
        if (!transaction.commit()) {
            LOG_ERROR("Failed to commit migration " + std::to_string(migration.version));
            return false;
        }
        
        LOG_INFO("Migration " + std::to_string(migration.version) + " completed successfully");
    }
    
    return true;
}

int DatabaseManager::getCurrentVersion() {
    auto stmt = prepare("SELECT version FROM schema_version ORDER BY version DESC LIMIT 1");
    if (!stmt || !stmt->isValid()) {
        return 0;
    }
    
    if (stmt->step()) {
        return stmt->getColumnInt(0);
    }
    
    return 0;
}

bool DatabaseManager::setVersion(int version) {
    auto stmt = prepare("INSERT INTO schema_version (version, applied_at) VALUES (?, datetime('now'))");
    if (!stmt || !stmt->isValid()) {
        return false;
    }
    
    stmt->bindInt(1, version);
    return stmt->step();
}

bool DatabaseManager::vacuum() {
    return execute("VACUUM");
}

bool DatabaseManager::backup(const std::string& backupPath) {
    sqlite3* backupDb = nullptr;
    int result = sqlite3_open(backupPath.c_str(), &backupDb);
    
    if (result != SQLITE_OK) {
        LOG_ERROR("Failed to create backup database: " + std::string(sqlite3_errmsg(backupDb)));
        if (backupDb) {
            sqlite3_close(backupDb);
        }
        return false;
    }
    
    sqlite3_backup* backup = sqlite3_backup_init(backupDb, "main", db_, "main");
    if (!backup) {
        LOG_ERROR("Failed to initialize backup");
        sqlite3_close(backupDb);
        return false;
    }
    
    result = sqlite3_backup_step(backup, -1);
    sqlite3_backup_finish(backup);
    sqlite3_close(backupDb);
    
    if (result != SQLITE_DONE) {
        LOG_ERROR("Backup failed: " + std::string(sqlite3_errmsg(db_)));
        return false;
    }
    
    LOG_INFO("Database backup created: " + backupPath);
    return true;
}

bool DatabaseManager::restore(const std::string& backupPath) {
    if (!fileExists(backupPath)) {
        LOG_ERROR("Backup file does not exist: " + backupPath);
        return false;
    }
    
    close();
    
    try {
        std::filesystem::copy_file(backupPath, dbPath_, std::filesystem::copy_options::overwrite_existing);
        return open(dbPath_);
    } catch (const std::filesystem::filesystem_error& e) {
        LOG_ERROR("Failed to restore backup: " + std::string(e.what()));
        return false;
    }
}

int64_t DatabaseManager::getDatabaseSize() {
    if (!isOpen_) {
        return 0;
    }
    
    try {
        return std::filesystem::file_size(dbPath_);
    } catch (const std::filesystem::filesystem_error&) {
        return 0;
    }
}

std::string DatabaseManager::getLastError() const {
    if (db_) {
        return std::string(sqlite3_errmsg(db_));
    }
    return "Database not open";
}

int64_t DatabaseManager::getLastInsertRowId() const {
    if (db_) {
        return sqlite3_last_insert_rowid(db_);
    }
    return 0;
}

int DatabaseManager::getChanges() const {
    if (db_) {
        return sqlite3_changes(db_);
    }
    return 0;
}

std::string DatabaseManager::escapeString(const std::string& str) {
    std::string escaped;
    escaped.reserve(str.length() * 2);
    
    for (char c : str) {
        if (c == '\'') {
            escaped += "''";
        } else {
            escaped += c;
        }
    }
    
    return escaped;
}

bool DatabaseManager::fileExists(const std::string& path) {
    return std::filesystem::exists(path);
}

bool DatabaseManager::createDirectoryIfNotExists(const std::string& path) {
    try {
        return std::filesystem::create_directories(path);
    } catch (const std::filesystem::filesystem_error&) {
        return false;
    }
}

void DatabaseManager::initializeMigrations() {
    // Migration 1: Create basic schema
    addMigration(Migration(1, "Create basic schema", {
        R"(
        CREATE TABLE IF NOT EXISTS schema_version (
            version INTEGER PRIMARY KEY,
            applied_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
        )",
        R"(
        CREATE TABLE IF NOT EXISTS projects (
            id TEXT PRIMARY KEY,
            name TEXT NOT NULL,
            description TEXT,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            room_width REAL NOT NULL,
            room_height REAL NOT NULL,
            room_depth REAL NOT NULL,
            scene_data TEXT,
            metadata TEXT,
            thumbnail_path TEXT,
            object_count INTEGER DEFAULT 0
        )
        )",
        R"(
        CREATE TABLE IF NOT EXISTS catalog_items (
            id TEXT PRIMARY KEY,
            name TEXT NOT NULL,
            category TEXT NOT NULL,
            width REAL NOT NULL,
            height REAL NOT NULL,
            depth REAL NOT NULL,
            base_price REAL NOT NULL DEFAULT 0.0,
            model_path TEXT,
            thumbnail_path TEXT,
            specifications TEXT,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
        )",
        R"(
        CREATE TABLE IF NOT EXISTS material_options (
            id TEXT PRIMARY KEY,
            catalog_item_id TEXT NOT NULL,
            name TEXT NOT NULL,
            texture_path TEXT,
            price_modifier REAL DEFAULT 0.0,
            properties TEXT,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (catalog_item_id) REFERENCES catalog_items(id) ON DELETE CASCADE
        )
        )",
        R"(
        CREATE TABLE IF NOT EXISTS scene_objects (
            id TEXT PRIMARY KEY,
            project_id TEXT NOT NULL,
            catalog_item_id TEXT NOT NULL,
            position_x REAL NOT NULL,
            position_y REAL NOT NULL,
            position_z REAL NOT NULL,
            rotation_x REAL DEFAULT 0.0,
            rotation_y REAL DEFAULT 0.0,
            rotation_z REAL DEFAULT 0.0,
            scale_x REAL DEFAULT 1.0,
            scale_y REAL DEFAULT 1.0,
            scale_z REAL DEFAULT 1.0,
            material_id TEXT,
            custom_properties TEXT,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (project_id) REFERENCES projects(id) ON DELETE CASCADE,
            FOREIGN KEY (catalog_item_id) REFERENCES catalog_items(id)
        )
        )",
        R"(
        CREATE TABLE IF NOT EXISTS user_config (
            key TEXT PRIMARY KEY,
            value TEXT NOT NULL,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
        )"
    }));
    
    // Migration 2: Add indexes for performance
    addMigration(Migration(2, "Add performance indexes", {
        "CREATE INDEX IF NOT EXISTS idx_projects_updated ON projects(updated_at DESC)",
        "CREATE INDEX IF NOT EXISTS idx_projects_name ON projects(name)",
        "CREATE INDEX IF NOT EXISTS idx_catalog_category ON catalog_items(category)",
        "CREATE INDEX IF NOT EXISTS idx_catalog_name ON catalog_items(name)",
        "CREATE INDEX IF NOT EXISTS idx_scene_objects_project ON scene_objects(project_id)",
        "CREATE INDEX IF NOT EXISTS idx_scene_objects_catalog ON scene_objects(catalog_item_id)",
        "CREATE INDEX IF NOT EXISTS idx_material_options_catalog ON material_options(catalog_item_id)"
    }));
    
    // Migration 3: Add auto-save support
    addMigration(Migration(3, "Add auto-save support", {
        R"(
        CREATE TABLE IF NOT EXISTS auto_save_config (
            project_id TEXT PRIMARY KEY,
            enabled BOOLEAN DEFAULT 0,
            interval_seconds INTEGER DEFAULT 30,
            last_save DATETIME,
            FOREIGN KEY (project_id) REFERENCES projects(id) ON DELETE CASCADE
        )
        )",
        "CREATE INDEX IF NOT EXISTS idx_auto_save_enabled ON auto_save_config(enabled)"
    }));
}

bool DatabaseManager::createMigrationTable() {
    return execute(R"(
        CREATE TABLE IF NOT EXISTS schema_version (
            version INTEGER PRIMARY KEY,
            applied_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )");
}

void DatabaseManager::cleanup() {
    close();
}

// PreparedStatement implementation
DatabaseManager::PreparedStatement::PreparedStatement(sqlite3* db, const std::string& sql)
    : stmt_(nullptr), isValid_(false) {
    int result = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt_, nullptr);
    if (result == SQLITE_OK) {
        isValid_ = true;
    } else {
        LOG_ERROR("Failed to prepare statement: " + std::string(sqlite3_errmsg(db)) + " (SQL: " + sql + ")");
    }
}

DatabaseManager::PreparedStatement::~PreparedStatement() {
    if (stmt_) {
        sqlite3_finalize(stmt_);
    }
}

bool DatabaseManager::PreparedStatement::bindText(int index, const std::string& value) {
    if (!isValid_) return false;
    return sqlite3_bind_text(stmt_, index, value.c_str(), -1, SQLITE_TRANSIENT) == SQLITE_OK;
}

bool DatabaseManager::PreparedStatement::bindInt(int index, int value) {
    if (!isValid_) return false;
    return sqlite3_bind_int(stmt_, index, value) == SQLITE_OK;
}

bool DatabaseManager::PreparedStatement::bindInt64(int index, int64_t value) {
    if (!isValid_) return false;
    return sqlite3_bind_int64(stmt_, index, value) == SQLITE_OK;
}

bool DatabaseManager::PreparedStatement::bindDouble(int index, double value) {
    if (!isValid_) return false;
    return sqlite3_bind_double(stmt_, index, value) == SQLITE_OK;
}

bool DatabaseManager::PreparedStatement::bindNull(int index) {
    if (!isValid_) return false;
    return sqlite3_bind_null(stmt_, index) == SQLITE_OK;
}

bool DatabaseManager::PreparedStatement::step() {
    if (!isValid_) return false;
    int result = sqlite3_step(stmt_);
    return result == SQLITE_ROW || result == SQLITE_DONE;
}

bool DatabaseManager::PreparedStatement::reset() {
    if (!isValid_) return false;
    return sqlite3_reset(stmt_) == SQLITE_OK;
}

std::string DatabaseManager::PreparedStatement::getColumnText(int index) const {
    if (!isValid_) return "";
    const char* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt_, index));
    return text ? text : "";
}

int DatabaseManager::PreparedStatement::getColumnInt(int index) const {
    if (!isValid_) return 0;
    return sqlite3_column_int(stmt_, index);
}

int64_t DatabaseManager::PreparedStatement::getColumnInt64(int index) const {
    if (!isValid_) return 0;
    return sqlite3_column_int64(stmt_, index);
}

double DatabaseManager::PreparedStatement::getColumnDouble(int index) const {
    if (!isValid_) return 0.0;
    return sqlite3_column_double(stmt_, index);
}

bool DatabaseManager::PreparedStatement::isColumnNull(int index) const {
    if (!isValid_) return true;
    return sqlite3_column_type(stmt_, index) == SQLITE_NULL;
}

int DatabaseManager::PreparedStatement::getColumnCount() const {
    if (!isValid_) return 0;
    return sqlite3_column_count(stmt_);
}

std::string DatabaseManager::PreparedStatement::getColumnName(int index) const {
    if (!isValid_) return "";
    const char* name = sqlite3_column_name(stmt_, index);
    return name ? name : "";
}

// Transaction implementation
Transaction::Transaction(DatabaseManager& db) : db_(db), committed_(false) {
    db_.beginTransaction();
}

Transaction::~Transaction() {
    if (!committed_) {
        db_.rollbackTransaction();
    }
}

bool Transaction::commit() {
    if (!committed_) {
        committed_ = db_.commitTransaction();
    }
    return committed_;
}

void Transaction::rollback() {
    if (!committed_) {
        db_.rollbackTransaction();
        committed_ = true; // Prevent destructor from rolling back again
    }
}

} // namespace Persistence
} // namespace KitchenCAD