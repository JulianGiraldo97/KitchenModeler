#pragma once

#include "DatabaseManager.h"
#include "../models/CatalogItem.h"
#include <memory>
#include <vector>
#include <optional>
#include <mutex>
#include <limits>

namespace KitchenCAD {

// Forward declarations
namespace Models {
    class CatalogItem;
    struct MaterialOption;
    struct CatalogFilter;
    struct CatalogSearchResult;
}

// Use aliases for convenience
using CatalogItem = Models::CatalogItem;
using MaterialOption = Models::MaterialOption;
using CatalogFilter = Models::CatalogFilter;
using CatalogSearchResult = Models::CatalogSearchResult;

namespace Persistence {

/**
 * @brief Interface for catalog persistence operations
 */
class ICatalogRepository {
public:
    virtual ~ICatalogRepository() = default;
    
    // Catalog item CRUD operations
    virtual bool addItem(const Models::CatalogItem& item) = 0;
    virtual std::optional<Models::CatalogItem> getItem(const std::string& itemId) = 0;
    virtual bool updateItem(const Models::CatalogItem& item) = 0;
    virtual bool deleteItem(const std::string& itemId) = 0;
    
    // Catalog queries
    virtual std::vector<Models::CatalogItem> getAllItems() = 0;
    virtual std::vector<Models::CatalogItem> getItemsByCategory(const std::string& category) = 0;
    virtual Models::CatalogSearchResult searchItems(const Models::CatalogFilter& filter, size_t offset = 0, size_t limit = 50) = 0;
    virtual std::vector<std::string> getCategories() = 0;
    
    // Material options
    virtual bool addMaterialOption(const std::string& itemId, const Models::MaterialOption& option) = 0;
    virtual bool updateMaterialOption(const Models::MaterialOption& option) = 0;
    virtual bool deleteMaterialOption(const std::string& optionId) = 0;
    virtual std::vector<Models::MaterialOption> getMaterialOptions(const std::string& itemId) = 0;
    
    // Bulk operations
    virtual bool importCatalog(const std::vector<Models::CatalogItem>& items) = 0;
    virtual std::vector<Models::CatalogItem> exportCatalog() = 0;
    virtual bool clearCatalog() = 0;
    
    // Statistics
    virtual size_t getItemCount() = 0;
    virtual size_t getItemCountByCategory(const std::string& category) = 0;
    
    // Validation
    virtual bool itemExists(const std::string& itemId) = 0;
    virtual bool isValidItemName(const std::string& name) = 0;
};

/**
 * @brief SQLite implementation of catalog repository
 */
class SQLiteCatalogRepository : public ICatalogRepository {
private:
    std::unique_ptr<DatabaseManager> db_;
    mutable std::mutex mutex_;
    
public:
    explicit SQLiteCatalogRepository(const std::string& databasePath);
    explicit SQLiteCatalogRepository(std::unique_ptr<DatabaseManager> db);
    ~SQLiteCatalogRepository() override = default;
    
    // Non-copyable and non-movable (due to mutex)
    SQLiteCatalogRepository(const SQLiteCatalogRepository&) = delete;
    SQLiteCatalogRepository& operator=(const SQLiteCatalogRepository&) = delete;
    SQLiteCatalogRepository(SQLiteCatalogRepository&&) = delete;
    SQLiteCatalogRepository& operator=(SQLiteCatalogRepository&&) = delete;
    
    // Catalog item CRUD operations
    bool addItem(const Models::CatalogItem& item) override;
    std::optional<Models::CatalogItem> getItem(const std::string& itemId) override;
    bool updateItem(const Models::CatalogItem& item) override;
    bool deleteItem(const std::string& itemId) override;
    
    // Catalog queries
    std::vector<Models::CatalogItem> getAllItems() override;
    std::vector<Models::CatalogItem> getItemsByCategory(const std::string& category) override;
    Models::CatalogSearchResult searchItems(const Models::CatalogFilter& filter, size_t offset = 0, size_t limit = 50) override;
    std::vector<std::string> getCategories() override;
    
    // Material options
    bool addMaterialOption(const std::string& itemId, const Models::MaterialOption& option) override;
    bool updateMaterialOption(const Models::MaterialOption& option) override;
    bool deleteMaterialOption(const std::string& optionId) override;
    std::vector<Models::MaterialOption> getMaterialOptions(const std::string& itemId) override;
    
    // Bulk operations
    bool importCatalog(const std::vector<Models::CatalogItem>& items) override;
    std::vector<Models::CatalogItem> exportCatalog() override;
    bool clearCatalog() override;
    
    // Statistics
    size_t getItemCount() override;
    size_t getItemCountByCategory(const std::string& category) override;
    
    // Validation
    bool itemExists(const std::string& itemId) override;
    bool isValidItemName(const std::string& name) override;
    
    // Additional utility methods
    DatabaseManager* getDatabase() const { return db_.get(); }
    bool isConnected() const { return db_ && db_->isOpen(); }
    
private:
    // Helper methods for database operations
    bool insertCatalogItem(const Models::CatalogItem& item);
    bool updateCatalogItem(const Models::CatalogItem& item);
    std::optional<Models::CatalogItem> loadCatalogItemFromDatabase(const std::string& itemId);
    bool loadMaterialOptionsForItem(Models::CatalogItem& item);
    
    // Query building helpers
    std::string buildSearchQuery(const Models::CatalogFilter& filter, bool countOnly = false) const;
    void bindSearchParameters(DatabaseManager::PreparedStatement& stmt, const Models::CatalogFilter& filter) const;
    
    // Conversion helpers
    Models::CatalogItem resultToCatalogItem(DatabaseManager::PreparedStatement& stmt) const;
    Models::MaterialOption resultToMaterialOption(DatabaseManager::PreparedStatement& stmt) const;
    
    // Utility methods
    std::string formatTimestamp(const std::chrono::system_clock::time_point& timePoint) const;
    std::chrono::system_clock::time_point parseTimestamp(const std::string& timestamp) const;
};

} // namespace Persistence
} // namespace KitchenCAD