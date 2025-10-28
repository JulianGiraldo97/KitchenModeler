#pragma once

#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <sqlite3.h>

namespace KitchenCAD {
namespace Persistence {

/**
 * @brief Database migration information
 */
struct Migration {
    int version;
    std::string description;
    std::vector<std::string> sqlStatements;
    
    Migration(int v, const std::string& desc, const std::vector<std::string>& sql)
        : version(v), description(desc), sqlStatements(sql) {}
};

/**
 * @brief Database connection and management class
 * 
 * Handles SQLite database connections, migrations, and basic operations
 */
class DatabaseManager {
private:
    sqlite3* db_;
    std::string dbPath_;
    bool isOpen_;
    std::vector<Migration> migrations_;
    
public:
    DatabaseManager();
    explicit DatabaseManager(const std::string& dbPath);
    ~DatabaseManager();
    
    // Non-copyable and non-movable
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;
    DatabaseManager(DatabaseManager&&) = delete;
    DatabaseManager& operator=(DatabaseManager&&) = delete;
    
    // Connection management
    bool open(const std::string& dbPath);
    void close();
    bool isOpen() const { return isOpen_; }
    const std::string& getPath() const { return dbPath_; }
    
    // Transaction management
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();
    
    // Query execution
    bool execute(const std::string& sql);
    bool execute(const std::string& sql, const std::vector<std::string>& params);
    
    // Prepared statements
    class PreparedStatement {
    private:
        sqlite3_stmt* stmt_;
        bool isValid_;
        
    public:
        PreparedStatement(sqlite3* db, const std::string& sql);
        ~PreparedStatement();
        
        // Non-copyable and non-movable
        PreparedStatement(const PreparedStatement&) = delete;
        PreparedStatement& operator=(const PreparedStatement&) = delete;
        PreparedStatement(PreparedStatement&&) = delete;
        PreparedStatement& operator=(PreparedStatement&&) = delete;
        
        bool isValid() const { return isValid_; }
        
        // Parameter binding
        bool bindText(int index, const std::string& value);
        bool bindInt(int index, int value);
        bool bindInt64(int index, int64_t value);
        bool bindDouble(int index, double value);
        bool bindNull(int index);
        
        // Execution
        bool step();
        bool reset();
        
        // Result retrieval
        std::string getColumnText(int index) const;
        int getColumnInt(int index) const;
        int64_t getColumnInt64(int index) const;
        double getColumnDouble(int index) const;
        bool isColumnNull(int index) const;
        int getColumnCount() const;
        std::string getColumnName(int index) const;
    };
    
    std::unique_ptr<PreparedStatement> prepare(const std::string& sql);
    
    // Migration system
    void addMigration(const Migration& migration);
    bool runMigrations();
    int getCurrentVersion();
    bool setVersion(int version);
    
    // Database maintenance
    bool vacuum();
    bool backup(const std::string& backupPath);
    bool restore(const std::string& backupPath);
    int64_t getDatabaseSize();
    
    // Utility functions
    std::string getLastError() const;
    int64_t getLastInsertRowId() const;
    int getChanges() const;
    
    // Static utility functions
    static std::string escapeString(const std::string& str);
    static bool fileExists(const std::string& path);
    static bool createDirectoryIfNotExists(const std::string& path);
    
private:
    void initializeMigrations();
    bool createMigrationTable();
    void cleanup();
};

/**
 * @brief RAII transaction helper
 */
class Transaction {
private:
    DatabaseManager& db_;
    bool committed_;
    
public:
    explicit Transaction(DatabaseManager& db);
    ~Transaction();
    
    bool commit();
    void rollback();
    
    // Non-copyable and non-movable
    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;
    Transaction(Transaction&&) = delete;
    Transaction& operator=(Transaction&&) = delete;
};

} // namespace Persistence
} // namespace KitchenCAD