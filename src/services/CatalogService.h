#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include "../models/CatalogItem.h"

namespace KitchenCAD {
namespace Services {

using namespace Models;

/**
 * @brief Catalog import/export options
 */
struct ImportExportOptions {
    bool includeImages = true;
    bool includeMaterials = true;
    bool overwriteExisting = false;
    std::string imageBasePath = "catalog/images";
    std::string modelBasePath = "catalog/models";
};

/**
 * @brief Catalog management service
 * 
 * This service manages catalog items, categories, search, filtering,
 * and catalog operations. Implements requirements 3.1, 3.2, 12.1, 12.2, 12.4, 12.5
 */
class CatalogService {
public:
    
    /**
     * @brief Catalog statistics
     */
    struct CatalogStatistics {
        size_t totalItems = 0;
        std::unordered_map<std::string, size_t> itemsByCategory;
        double averagePrice = 0.0;
        double minPrice = 0.0;
        double maxPrice = 0.0;
        size_t itemsWithImages = 0;
        size_t itemsWithModels = 0;
    };

private:
    std::unordered_map<std::string, std::shared_ptr<CatalogItem>> items_;
    std::unordered_map<std::string, std::vector<std::string>> categorizedItems_;
    std::unordered_set<std::string> categories_;
    
    // Search index for fast text search
    std::unordered_map<std::string, std::unordered_set<std::string>> searchIndex_;
    
    // Configuration
    std::string catalogBasePath_;
    ImportExportOptions defaultImportOptions_;
    
    // Callbacks
    std::function<void(const std::string&)> itemAddedCallback_;
    std::function<void(const std::string&)> itemRemovedCallback_;
    std::function<void(const std::string&)> itemUpdatedCallback_;
    std::function<void(const std::string&)> errorCallback_;

public:
    /**
     * @brief Constructor
     */
    explicit CatalogService(const std::string& catalogBasePath = "catalog");
    
    /**
     * @brief Destructor
     */
    ~CatalogService() = default;
    
    // Item management
    
    /**
     * @brief Add item to catalog
     */
    bool addItem(std::shared_ptr<CatalogItem> item);
    
    /**
     * @brief Remove item from catalog
     */
    bool removeItem(const std::string& itemId);
    
    /**
     * @brief Update existing item
     */
    bool updateItem(std::shared_ptr<CatalogItem> item);
    
    /**
     * @brief Get item by ID
     */
    std::shared_ptr<CatalogItem> getItem(const std::string& itemId) const;
    
    /**
     * @brief Check if item exists
     */
    bool hasItem(const std::string& itemId) const;
    
    /**
     * @brief Get all items
     */
    std::vector<std::shared_ptr<CatalogItem>> getAllItems() const;
    
    /**
     * @brief Get item count
     */
    size_t getItemCount() const { return items_.size(); }
    
    // Category management
    
    /**
     * @brief Get all categories
     */
    std::vector<std::string> getCategories() const;
    
    /**
     * @brief Get items in category
     */
    std::vector<std::shared_ptr<CatalogItem>> getItemsByCategory(const std::string& category) const;
    
    /**
     * @brief Add custom category
     */
    void addCategory(const std::string& category);
    
    /**
     * @brief Remove category (moves items to "uncategorized")
     */
    bool removeCategory(const std::string& category);
    
    /**
     * @brief Rename category
     */
    bool renameCategory(const std::string& oldName, const std::string& newName);
    
    /**
     * @brief Get item count by category
     */
    std::unordered_map<std::string, size_t> getCategoryCounts() const;
    
    // Search and filtering
    
    /**
     * @brief Search items by text
     */
    CatalogSearchResult searchItems(const std::string& searchTerm, 
                                   size_t offset = 0, size_t limit = 50) const;
    
    /**
     * @brief Filter items by criteria
     */
    CatalogSearchResult filterItems(const CatalogFilter& filter,
                                   size_t offset = 0, size_t limit = 50) const;
    
    /**
     * @brief Advanced search with multiple criteria
     */
    CatalogSearchResult advancedSearch(const std::string& searchTerm,
                                      const CatalogFilter& filter,
                                      size_t offset = 0, size_t limit = 50) const;
    
    /**
     * @brief Get similar items (by category and dimensions)
     */
    std::vector<std::shared_ptr<CatalogItem>> getSimilarItems(const std::string& itemId,
                                                             size_t maxResults = 10) const;
    
    /**
     * @brief Get recommended items based on usage patterns
     */
    std::vector<std::shared_ptr<CatalogItem>> getRecommendedItems(const std::string& category = "",
                                                                 size_t maxResults = 10) const;
    
    // Import/Export operations
    
    /**
     * @brief Import catalog from JSON file
     */
    bool importFromJson(const std::string& filePath, 
                       const ImportExportOptions& options = ImportExportOptions());
    
    /**
     * @brief Export catalog to JSON file
     */
    bool exportToJson(const std::string& filePath,
                     const ImportExportOptions& options = ImportExportOptions()) const;
    
    /**
     * @brief Import single item from JSON
     */
    std::shared_ptr<CatalogItem> importItemFromJson(const nlohmann::json& itemJson,
                                                   const ImportExportOptions& options = ImportExportOptions());
    
    /**
     * @brief Export items by category
     */
    bool exportCategoryToJson(const std::string& category, const std::string& filePath,
                             const ImportExportOptions& options = ImportExportOptions()) const;
    
    /**
     * @brief Import catalog from CSV file
     */
    bool importFromCsv(const std::string& filePath,
                      const ImportExportOptions& options = ImportExportOptions());
    
    /**
     * @brief Export catalog to CSV file
     */
    bool exportToCsv(const std::string& filePath) const;
    
    // Validation and maintenance
    
    /**
     * @brief Validate all catalog items
     */
    std::vector<std::string> validateCatalog() const;
    
    /**
     * @brief Validate specific item
     */
    std::vector<std::string> validateItem(const std::string& itemId) const;
    
    /**
     * @brief Check for duplicate items
     */
    std::vector<std::pair<std::string, std::string>> findDuplicates() const;
    
    /**
     * @brief Clean up orphaned files
     */
    std::vector<std::string> cleanupOrphanedFiles() const;
    
    /**
     * @brief Rebuild search index
     */
    void rebuildSearchIndex();
    
    /**
     * @brief Optimize catalog (remove unused categories, etc.)
     */
    void optimize();
    
    // Statistics and analysis
    
    /**
     * @brief Get catalog statistics
     */
    CatalogStatistics getStatistics() const;
    
    /**
     * @brief Get price statistics by category
     */
    std::unordered_map<std::string, std::pair<double, double>> getPriceRangesByCategory() const;
    
    /**
     * @brief Get most popular items (would need usage tracking)
     */
    std::vector<std::shared_ptr<CatalogItem>> getMostPopularItems(size_t maxResults = 10) const;
    
    /**
     * @brief Get recently added items
     */
    std::vector<std::shared_ptr<CatalogItem>> getRecentlyAddedItems(size_t maxResults = 10) const;
    
    // Configuration
    
    /**
     * @brief Set catalog base path
     */
    void setCatalogBasePath(const std::string& path) { catalogBasePath_ = path; }
    
    /**
     * @brief Get catalog base path
     */
    const std::string& getCatalogBasePath() const { return catalogBasePath_; }
    
    /**
     * @brief Set default import options
     */
    void setDefaultImportOptions(const ImportExportOptions& options) {
        defaultImportOptions_ = options;
    }
    
    /**
     * @brief Get default import options
     */
    const ImportExportOptions& getDefaultImportOptions() const { return defaultImportOptions_; }
    
    // Callbacks
    
    /**
     * @brief Set callback for item added events
     */
    void setItemAddedCallback(std::function<void(const std::string&)> callback) {
        itemAddedCallback_ = callback;
    }
    
    /**
     * @brief Set callback for item removed events
     */
    void setItemRemovedCallback(std::function<void(const std::string&)> callback) {
        itemRemovedCallback_ = callback;
    }
    
    /**
     * @brief Set callback for item updated events
     */
    void setItemUpdatedCallback(std::function<void(const std::string&)> callback) {
        itemUpdatedCallback_ = callback;
    }
    
    /**
     * @brief Set callback for errors
     */
    void setErrorCallback(std::function<void(const std::string&)> callback) {
        errorCallback_ = callback;
    }
    
    // Bulk operations
    
    /**
     * @brief Add multiple items
     */
    size_t addItems(const std::vector<std::shared_ptr<CatalogItem>>& items);
    
    /**
     * @brief Remove multiple items
     */
    size_t removeItems(const std::vector<std::string>& itemIds);
    
    /**
     * @brief Update multiple items
     */
    size_t updateItems(const std::vector<std::shared_ptr<CatalogItem>>& items);
    
    /**
     * @brief Clear all items
     */
    void clear();
    
    // Utility methods
    
    /**
     * @brief Generate unique item ID
     */
    std::string generateItemId(const std::string& category = "") const;
    
    /**
     * @brief Validate item ID format
     */
    static bool isValidItemId(const std::string& itemId);
    
    /**
     * @brief Normalize category name
     */
    static std::string normalizeCategory(const std::string& category);
    
    /**
     * @brief Get standard kitchen categories
     */
    static std::vector<std::string> getStandardCategories();

private:
    /**
     * @brief Update category index when item is added/removed
     */
    void updateCategoryIndex(const std::string& itemId, const std::string& category, bool add);
    
    /**
     * @brief Update search index for item
     */
    void updateSearchIndex(const std::shared_ptr<CatalogItem>& item, bool add);
    
    /**
     * @brief Extract search terms from item
     */
    std::vector<std::string> extractSearchTerms(const CatalogItem& item) const;
    
    /**
     * @brief Normalize search term
     */
    std::string normalizeSearchTerm(const std::string& term) const;
    
    /**
     * @brief Check if items match (for duplicate detection)
     */
    bool itemsMatch(const CatalogItem& item1, const CatalogItem& item2) const;
    
    /**
     * @brief Notify callbacks
     */
    void notifyItemAdded(const std::string& itemId);
    void notifyItemRemoved(const std::string& itemId);
    void notifyItemUpdated(const std::string& itemId);
    void notifyError(const std::string& error) const;
    
    /**
     * @brief Initialize standard categories
     */
    void initializeStandardCategories();
    
    /**
     * @brief Validate file paths in item
     */
    bool validateItemPaths(const CatalogItem& item) const;
};

} // namespace Services
} // namespace KitchenCAD