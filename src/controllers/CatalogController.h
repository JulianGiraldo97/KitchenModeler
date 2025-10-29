#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include "../services/CatalogService.h"
#include "../models/CatalogItem.h"
#include <unordered_map>
#include <limits>

namespace KitchenCAD {
namespace Controllers {

using namespace Models;
using namespace Services;

/**
 * @brief Catalog operation result
 */
struct CatalogOperationResult {
    bool success = false;
    std::string message;
    std::string itemId;
    
    CatalogOperationResult() = default;
    CatalogOperationResult(bool success, const std::string& message, const std::string& id = "")
        : success(success), message(message), itemId(id) {}
};

/**
 * @brief Catalog controller for managing catalog operations
 * 
 * This controller handles all catalog-related operations including item management,
 * search, filtering, and catalog maintenance. Implements requirements 3.1, 3.2, 12.1, 12.2.
 */
class CatalogController {
private:
    std::unique_ptr<CatalogService> catalogService_;
    
    // Callbacks
    std::function<void(const std::string&)> itemAddedCallback_;
    std::function<void(const std::string&)> itemRemovedCallback_;
    std::function<void(const std::string&)> itemUpdatedCallback_;
    std::function<void(const std::string&)> catalogLoadedCallback_;
    std::function<void(const std::string&)> errorCallback_;

public:
    /**
     * @brief Constructor
     */
    explicit CatalogController(std::unique_ptr<CatalogService> catalogService);
    
    /**
     * @brief Destructor
     */
    ~CatalogController() = default;
    
    // Item management
    
    /**
     * @brief Add new item to catalog
     */
    CatalogOperationResult addItem(const std::string& id, const std::string& name, 
                                  const std::string& category, const Dimensions3D& dimensions,
                                  double basePrice = 0.0);
    
    /**
     * @brief Add item from CatalogItem object
     */
    CatalogOperationResult addItem(std::shared_ptr<CatalogItem> item);
    
    /**
     * @brief Remove item from catalog
     */
    CatalogOperationResult removeItem(const std::string& itemId);
    
    /**
     * @brief Update existing item
     */
    CatalogOperationResult updateItem(std::shared_ptr<CatalogItem> item);
    
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
    size_t getItemCount() const;
    
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
    bool addCategory(const std::string& category);
    
    /**
     * @brief Remove category
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
    
    /**
     * @brief Get standard kitchen categories
     */
    std::vector<std::string> getStandardCategories() const;
    
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
     * @brief Get similar items
     */
    std::vector<std::shared_ptr<CatalogItem>> getSimilarItems(const std::string& itemId,
                                                             size_t maxResults = 10) const;
    
    /**
     * @brief Get recommended items
     */
    std::vector<std::shared_ptr<CatalogItem>> getRecommendedItems(const std::string& category = "",
                                                                 size_t maxResults = 10) const;
    
    /**
     * @brief Create filter from parameters
     */
    CatalogFilter createFilter(const std::vector<std::string>& categories = {},
                              const Dimensions3D& minDims = Dimensions3D(),
                              const Dimensions3D& maxDims = Dimensions3D(),
                              double minPrice = 0.0,
                              double maxPrice = std::numeric_limits<double>::max(),
                              const std::vector<std::string>& features = {}) const;
    
    // Import/Export operations
    
    /**
     * @brief Import catalog from JSON file
     */
    CatalogOperationResult importFromJson(const std::string& filePath, 
                                         bool includeImages = true,
                                         bool includeMaterials = true,
                                         bool overwriteExisting = false);
    
    /**
     * @brief Export catalog to JSON file
     */
    CatalogOperationResult exportToJson(const std::string& filePath,
                                       bool includeImages = true,
                                       bool includeMaterials = true) const;
    
    /**
     * @brief Import single item from JSON
     */
    CatalogOperationResult importItemFromJson(const nlohmann::json& itemJson,
                                             bool includeImages = true,
                                             bool includeMaterials = true);
    
    /**
     * @brief Export category to JSON file
     */
    CatalogOperationResult exportCategoryToJson(const std::string& category, 
                                               const std::string& filePath,
                                               bool includeImages = true,
                                               bool includeMaterials = true) const;
    
    /**
     * @brief Import catalog from CSV file
     */
    CatalogOperationResult importFromCsv(const std::string& filePath,
                                        bool includeImages = true,
                                        bool overwriteExisting = false);
    
    /**
     * @brief Export catalog to CSV file
     */
    CatalogOperationResult exportToCsv(const std::string& filePath) const;
    
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
     * @brief Optimize catalog
     */
    void optimizeCatalog();
    
    // Statistics and analysis
    
    /**
     * @brief Get catalog statistics
     */
    CatalogService::CatalogStatistics getStatistics() const;
    
    /**
     * @brief Get price statistics by category
     */
    std::unordered_map<std::string, std::pair<double, double>> getPriceRangesByCategory() const;
    
    /**
     * @brief Get most popular items
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
    void setCatalogBasePath(const std::string& path);
    
    /**
     * @brief Get catalog base path
     */
    std::string getCatalogBasePath() const;
    
    /**
     * @brief Set default import options
     */
    void setDefaultImportOptions(bool includeImages, bool includeMaterials, bool overwriteExisting);
    
    /**
     * @brief Get default import options
     */
    ImportExportOptions getDefaultImportOptions() const;
    
    // Bulk operations
    
    /**
     * @brief Add multiple items
     */
    CatalogOperationResult addItems(const std::vector<std::shared_ptr<CatalogItem>>& items);
    
    /**
     * @brief Remove multiple items
     */
    CatalogOperationResult removeItems(const std::vector<std::string>& itemIds);
    
    /**
     * @brief Update multiple items
     */
    CatalogOperationResult updateItems(const std::vector<std::shared_ptr<CatalogItem>>& items);
    
    /**
     * @brief Clear all items
     */
    CatalogOperationResult clearCatalog();
    
    // Item creation helpers
    
    /**
     * @brief Create basic cabinet item
     */
    std::shared_ptr<CatalogItem> createCabinetItem(const std::string& id, const std::string& name,
                                                  const std::string& subCategory,
                                                  const Dimensions3D& dimensions,
                                                  double basePrice) const;
    
    /**
     * @brief Create appliance item
     */
    std::shared_ptr<CatalogItem> createApplianceItem(const std::string& id, const std::string& name,
                                                    const std::string& brand,
                                                    const Dimensions3D& dimensions,
                                                    double basePrice) const;
    
    /**
     * @brief Create countertop item
     */
    std::shared_ptr<CatalogItem> createCountertopItem(const std::string& id, const std::string& name,
                                                     const std::string& material,
                                                     const Dimensions3D& dimensions,
                                                     double pricePerSquareMeter) const;
    
    /**
     * @brief Create hardware item
     */
    std::shared_ptr<CatalogItem> createHardwareItem(const std::string& id, const std::string& name,
                                                   const std::string& type,
                                                   double basePrice) const;
    
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
     * @brief Set callback for catalog loaded events
     */
    void setCatalogLoadedCallback(std::function<void(const std::string&)> callback) {
        catalogLoadedCallback_ = callback;
    }
    
    /**
     * @brief Set callback for errors
     */
    void setErrorCallback(std::function<void(const std::string&)> callback) {
        errorCallback_ = callback;
    }
    
    // Utility methods
    
    /**
     * @brief Generate unique item ID
     */
    std::string generateUniqueItemId(const std::string& category = "") const;
    
    /**
     * @brief Validate item ID format
     */
    static bool isValidItemId(const std::string& itemId);
    
    /**
     * @brief Normalize category name
     */
    static std::string normalizeCategory(const std::string& category);
    
    /**
     * @brief Check if item data is valid
     */
    bool isValidItemData(const std::string& name, const std::string& category, 
                        const Dimensions3D& dimensions, double basePrice) const;

private:
    /**
     * @brief Setup catalog service callbacks
     */
    void setupCallbacks();
    
    /**
     * @brief Notify callbacks
     */
    void notifyItemAdded(const std::string& itemId);
    void notifyItemRemoved(const std::string& itemId);
    void notifyItemUpdated(const std::string& itemId);
    void notifyCatalogLoaded(const std::string& message);
    void notifyError(const std::string& error);
    
    /**
     * @brief Create import/export options
     */
    ImportExportOptions createImportExportOptions(bool includeImages, bool includeMaterials, 
                                                 bool overwriteExisting = false) const;
    
    /**
     * @brief Validate import/export parameters
     */
    bool validateFilePath(const std::string& filePath, const std::string& expectedExtension) const;
    
    /**
     * @brief Create default specifications for item type
     */
    Specifications createDefaultSpecifications(const std::string& category) const;
};

} // namespace Controllers
} // namespace KitchenCAD