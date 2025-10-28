#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../src/persistence/DatabaseManager.h"
#include <filesystem>
#include <memory>
#include <fstream>

using namespace KitchenCAD::Persistence;
using Catch::Approx;

// Test database path
const std::string TEST_DB_PATH = "test_kitchen_cad_basic.db";

// Helper function to clean up test database
void cleanupTestDatabase() {
    if (std::filesystem::exists(TEST_DB_PATH)) {
        std::filesystem::remove(TEST_DB_PATH);
    }
}

TEST_CASE("DatabaseManager basic operations", "[persistence][database]") {
    cleanupTestDatabase();
    
    SECTION("Database creation and opening") {
        DatabaseManager db;
        REQUIRE(db.open(TEST_DB_PATH));
        REQUIRE(db.isOpen());
        REQUIRE(db.getPath() == TEST_DB_PATH);
        
        db.close();
        REQUIRE(!db.isOpen());
    }
    
    SECTION("SQL execution") {
        DatabaseManager db(TEST_DB_PATH);
        REQUIRE(db.isOpen());
        
        // Create a test table
        REQUIRE(db.execute("CREATE TABLE test_table (id INTEGER PRIMARY KEY, name TEXT)"));
        
        // Insert data
        REQUIRE(db.execute("INSERT INTO test_table (name) VALUES ('test')"));
        REQUIRE(db.getLastInsertRowId() > 0);
        REQUIRE(db.getChanges() == 1);
        
        // Query data
        auto stmt = db.prepare("SELECT id, name FROM test_table WHERE name = ?");
        REQUIRE(stmt != nullptr);
        REQUIRE(stmt->isValid());
        
        stmt->bindText(1, "test");
        REQUIRE(stmt->step());
        
        REQUIRE(stmt->getColumnInt(0) > 0);
        REQUIRE(stmt->getColumnText(1) == "test");
    }
    
    SECTION("Transaction management") {
        DatabaseManager db(TEST_DB_PATH);
        REQUIRE(db.isOpen());
        
        // Create test table
        REQUIRE(db.execute("CREATE TABLE test_table (id INTEGER PRIMARY KEY, value INTEGER)"));
        
        // Test successful transaction
        {
            Transaction transaction(db);
            REQUIRE(db.execute("INSERT INTO test_table (value) VALUES (1)"));
            REQUIRE(db.execute("INSERT INTO test_table (value) VALUES (2)"));
            REQUIRE(transaction.commit());
        }
        
        // Verify data was committed
        auto stmt = db.prepare("SELECT COUNT(*) FROM test_table");
        REQUIRE(stmt->step());
        REQUIRE(stmt->getColumnInt(0) == 2);
        
        // Test rollback transaction
        {
            Transaction transaction(db);
            REQUIRE(db.execute("INSERT INTO test_table (value) VALUES (3)"));
            // Don't commit - should rollback automatically
        }
        
        // Verify data was rolled back
        stmt->reset();
        REQUIRE(stmt->step());
        REQUIRE(stmt->getColumnInt(0) == 2);
    }
    
    SECTION("Migration system") {
        DatabaseManager db(TEST_DB_PATH);
        REQUIRE(db.isOpen());
        
        // Add a test migration
        Migration testMigration(100, "Test migration", {
            "CREATE TABLE test_migration (id INTEGER PRIMARY KEY, data TEXT)"
        });
        
        db.addMigration(testMigration);
        REQUIRE(db.runMigrations());
        
        // Verify migration was applied
        REQUIRE(db.getCurrentVersion() >= 100);
        
        // Verify table was created
        auto stmt = db.prepare("SELECT name FROM sqlite_master WHERE type='table' AND name='test_migration'");
        REQUIRE(stmt->step());
        REQUIRE(stmt->getColumnText(0) == "test_migration");
    }
    
    SECTION("Database maintenance") {
        DatabaseManager db(TEST_DB_PATH);
        REQUIRE(db.isOpen());
        
        // Test vacuum
        REQUIRE(db.vacuum());
        
        // Test backup
        const std::string BACKUP_PATH = "test_backup_basic.db";
        REQUIRE(db.backup(BACKUP_PATH));
        REQUIRE(std::filesystem::exists(BACKUP_PATH));
        
        // Clean up backup
        std::filesystem::remove(BACKUP_PATH);
    }
    
    cleanupTestDatabase();
}

TEST_CASE("PreparedStatement operations", "[persistence][statement]") {
    cleanupTestDatabase();
    
    DatabaseManager db(TEST_DB_PATH);
    REQUIRE(db.isOpen());
    
    // Create test table
    REQUIRE(db.execute("CREATE TABLE test_params (id INTEGER, name TEXT, price REAL, active BOOLEAN)"));
    
    SECTION("Parameter binding and execution") {
        auto stmt = db.prepare("INSERT INTO test_params (id, name, price, active) VALUES (?, ?, ?, ?)");
        REQUIRE(stmt != nullptr);
        REQUIRE(stmt->isValid());
        
        // Bind parameters
        REQUIRE(stmt->bindInt(1, 42));
        REQUIRE(stmt->bindText(2, "Test Item"));
        REQUIRE(stmt->bindDouble(3, 99.99));
        REQUIRE(stmt->bindInt(4, 1)); // Boolean as integer
        
        // Execute
        REQUIRE(stmt->step());
        
        // Verify insertion
        auto selectStmt = db.prepare("SELECT id, name, price, active FROM test_params WHERE id = ?");
        selectStmt->bindInt(1, 42);
        REQUIRE(selectStmt->step());
        
        REQUIRE(selectStmt->getColumnInt(0) == 42);
        REQUIRE(selectStmt->getColumnText(1) == "Test Item");
        REQUIRE(selectStmt->getColumnDouble(2) == Approx(99.99));
        REQUIRE(selectStmt->getColumnInt(3) == 1);
    }
    
    SECTION("NULL handling") {
        auto stmt = db.prepare("INSERT INTO test_params (id, name, price, active) VALUES (?, ?, ?, ?)");
        REQUIRE(stmt != nullptr);
        
        stmt->bindInt(1, 43);
        stmt->bindNull(2); // NULL name
        stmt->bindDouble(3, 50.0);
        stmt->bindInt(4, 0);
        
        REQUIRE(stmt->step());
        
        // Verify NULL handling
        auto selectStmt = db.prepare("SELECT name FROM test_params WHERE id = ?");
        selectStmt->bindInt(1, 43);
        REQUIRE(selectStmt->step());
        REQUIRE(selectStmt->isColumnNull(0));
    }
    
    SECTION("Statement reuse") {
        auto stmt = db.prepare("INSERT INTO test_params (id, name) VALUES (?, ?)");
        REQUIRE(stmt != nullptr);
        
        // First execution
        stmt->bindInt(1, 100);
        stmt->bindText(2, "First");
        REQUIRE(stmt->step());
        
        // Reset and reuse
        REQUIRE(stmt->reset());
        stmt->bindInt(1, 101);
        stmt->bindText(2, "Second");
        REQUIRE(stmt->step());
        
        // Verify both records
        auto countStmt = db.prepare("SELECT COUNT(*) FROM test_params WHERE id >= 100");
        REQUIRE(countStmt->step());
        REQUIRE(countStmt->getColumnInt(0) == 2);
    }
    
    cleanupTestDatabase();
}

TEST_CASE("Database error handling", "[persistence][error]") {
    cleanupTestDatabase();
    
    SECTION("Database operations on closed database") {
        DatabaseManager db;
        // Database should not be open initially
        REQUIRE_FALSE(db.isOpen());
        
        // Operations on closed database should fail
        REQUIRE_FALSE(db.execute("SELECT 1"));
        REQUIRE_FALSE(db.beginTransaction());
        REQUIRE_FALSE(db.commitTransaction());
        REQUIRE_FALSE(db.rollbackTransaction());
    }
    
    SECTION("SQL syntax errors") {
        DatabaseManager db(TEST_DB_PATH);
        REQUIRE(db.isOpen());
        
        // Invalid SQL should fail
        REQUIRE_FALSE(db.execute("INVALID SQL STATEMENT"));
        REQUIRE_FALSE(db.execute("SELECT * FROM non_existent_table"));
    }
    
    SECTION("Invalid prepared statements") {
        DatabaseManager db(TEST_DB_PATH);
        REQUIRE(db.isOpen());
        
        // Invalid SQL in prepared statement
        auto stmt = db.prepare("INVALID SQL");
        REQUIRE(stmt != nullptr);
        REQUIRE_FALSE(stmt->isValid());
    }
    
    SECTION("Transaction rollback on error") {
        DatabaseManager db(TEST_DB_PATH);
        REQUIRE(db.isOpen());
        
        // Create test table
        REQUIRE(db.execute("CREATE TABLE test_rollback (id INTEGER PRIMARY KEY, value INTEGER UNIQUE)"));
        
        // Insert initial data
        REQUIRE(db.execute("INSERT INTO test_rollback (value) VALUES (1)"));
        
        // Transaction that should fail due to constraint violation
        {
            Transaction transaction(db);
            REQUIRE(db.execute("INSERT INTO test_rollback (value) VALUES (2)"));
            // This should fail due to UNIQUE constraint
            REQUIRE_FALSE(db.execute("INSERT INTO test_rollback (value) VALUES (1)"));
            // Transaction will rollback automatically
        }
        
        // Verify rollback - should only have original record
        auto stmt = db.prepare("SELECT COUNT(*) FROM test_rollback");
        REQUIRE(stmt->step());
        REQUIRE(stmt->getColumnInt(0) == 1);
    }
    
    cleanupTestDatabase();
}

TEST_CASE("Database utility functions", "[persistence][utility]") {
    SECTION("String escaping") {
        std::string input = "Test's \"quoted\" string";
        std::string escaped = DatabaseManager::escapeString(input);
        
        // Should escape single quotes
        REQUIRE(escaped.find("''") != std::string::npos);
    }
    
    SECTION("File operations") {
        const std::string testFile = "test_file_ops.txt";
        
        // Initially should not exist
        REQUIRE_FALSE(DatabaseManager::fileExists(testFile));
        
        // Create file
        std::ofstream file(testFile);
        file << "test content";
        file.close();
        
        // Now should exist
        REQUIRE(DatabaseManager::fileExists(testFile));
        
        // Clean up
        std::filesystem::remove(testFile);
    }
    
    SECTION("Directory creation") {
        const std::string testDir = "test_directory_creation";
        
        // Clean up if exists
        if (std::filesystem::exists(testDir)) {
            std::filesystem::remove_all(testDir);
        }
        
        // Should not exist initially
        REQUIRE_FALSE(std::filesystem::exists(testDir));
        
        // Create directory
        REQUIRE(DatabaseManager::createDirectoryIfNotExists(testDir));
        REQUIRE(std::filesystem::exists(testDir));
        
        // Clean up
        std::filesystem::remove_all(testDir);
    }
}