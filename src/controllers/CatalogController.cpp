#include "CatalogController.h"
#include "../utils/Logger.h"
#include <algorithm>
#include <filesystem>
#include <regex>

namespace KitchenCAD {
namespace Controllers {

CatalogController::CatalogController(std::unique_ptr<CatalogService> catalogService)
    : catalogService_(std::move(catalogService))
{
    setupCallbacks();
}

CatalogOperationResult CatalogController::addItem(const std::string& id, const std::string& name, 
                                                 const std::string& category, const Dimensions3D& dimensions,
                                                 double basePrice) {
    if (!isValidItemData(name, category, dimensions, basePrice)) {
        return CatalogOperationResult(false, "Invalid item data");
    }
    
    if (!isValidItemId(id)) {
        return CatalogOperationResult(false, "Invalid item ID format: " + id);
    }
    
    if (hasItem(id)) {
        return CatalogOperationResult(false, "Item already exists: " + id);
    }
    
    try {
        auto item = std::make_shared<CatalogItem>(id, name, category);
        item->setDimensions(dimensions);
        item->setBasePrice(basePrice);
        item->setSpecifications(createDefaultSpecifications(category));
        
        bool success = catalogService_->addItem(item);
        
        if (!success) {
            return CatalogOperationResult(false, "Failed to add item to catalog");
        }
        
        notifyItemAdded(id);
        return CatalogOperationResult(true, "Item added successfully", id);
        
    } catch (const std::exception& e) {
        std::string error = "Failed to add item: " + std::string(e.what());
        notifyError(error);
        return CatalogOperationResult(false, error);
    }
}

CatalogOperationResult CatalogController::addItem(std::shared_ptr<CatalogItem> item) {
    if (!item) {
        return CatalogOperationResult(false, "Invalid item pointer");
    }
    
    if (!item->isValid()) {
        auto errors = item->validate();
        std::string errorMsg = "Item validation failed: ";
        for (const auto& error : errors) {
            errorMsg += error + "; ";
        }
        return CatalogOperationResult(false, errorMsg);
    }
    
    if (hasItem(item->getId())) {
        return CatalogOperationResult(false, "Item already exists: " + item->getId());
    }
    
    try {
        bool success = catalogService_->addItem(item);
        
        if (!success) {
            return CatalogOperationResult(false, "Failed to add item to catalog");
        }
        
        notifyItemAdded(item->getId());
        return CatalogOperationResult(true, "Item added successfully", item->getId());
        
    } catch (const std::exception& e) {
        std::string error = "Failed to add item: " + std::string(e.what());
        notifyError(error);
        return CatalogOperationResult(false, error);
    }
}

CatalogOperationResult CatalogController::removeItem(const std::string& itemId) {
    if (itemId.empty()) {
        return CatalogOperationResult(false, "Invalid item ID");
    }
    
    if (!hasItem(itemId)) {
        return CatalogOperationResult(false, "Item not found: " + itemId);
    }
    
    try {
        bool success = catalogService_->removeItem(itemId);
        
        if (!success) {
            return CatalogOperationResult(false, "Failed to remove item from catalog");
        }
        
        notifyItemRemoved(itemId);
        return CatalogOperationResult(true, "Item removed successfully", itemId);
        
    } catch (const std::exception& e) {
        std::string error = "Failed to remove item: " + std::string(e.what());
        notifyError(error);
        return CatalogOperationResult(false, error);
    }
}

CatalogOperationResult CatalogController::updateItem(std::shared_ptr<CatalogItem> item) {
    if (!item) {
        return CatalogOperationResult(false, "Invalid item pointer");
    }
    
    if (!item->isValid()) {
        auto errors = item->validate();
        std::string errorMsg = "Item validation failed: ";
        for (const auto& error : errors) {
            errorMsg += error + "; ";
        }
        return CatalogOperationResult(false, errorMsg);
    }
    
    if (!hasItem(item->getId())) {
        return CatalogOperationResult(false, "Item not found: " + item->getId());
    }
    
    try {
        bool success = catalogService_->updateItem(item);
        
        if (!success) {
            return CatalogOperationResult(false, "Failed to update item in catalog");
        }
        
        notifyItemUpdated(item->getId());
        return CatalogOperationResult(true, "Item updated successfully", item->getId());
        
    } catch (const std::exception& e) {
        std::string error = "Failed to update item: " + std::string(e.what());
        notifyError(error);
        return CatalogOperationResult(false, error);
    }
}

std::shared_ptr<CatalogItem> CatalogController::getItem(const std::string& itemId) const {
    if (catalogService_) {
        return catalogService_->getItem(itemId);
    }
    
    return nullptr;
}

bool CatalogController::hasItem(const std::string& itemId) const {
    if (catalogService_) {
        return catalogService_->hasItem(itemId);
    }
    
    return false;
}

std::vector<std::shared_ptr<CatalogItem>> CatalogController::getAllItems() const {
    if (catalogService_) {
        return catalogService_->getAllItems();
    }
    
    return {};
}

size_t CatalogController::getItemCount() const {
    if (catalogService_) {
        return catalogService_->getItemCount();
    }
    
    return 0;
}

std::vector<std::string> CatalogController::getCategories() const {
    if (catalogService_) {
        return catalogService_->getCategories();
    }
    
    return {};
}

std::vector<std::shared_ptr<CatalogItem>> CatalogController::getItemsByCategory(const std::string& category) const {
    if (catalogService_) {
        return catalogService_->getItemsByCategory(category);
    }
    
    return {};
}

bool CatalogController::addCategory(const std::string& category) {
    if (category.empty()) {
        notifyError("Category name cannot be empty");
        return false;
    }
    
    if (catalogService_) {
        catalogService_->addCategory(category);
        return true;
    }
    
    return false;
}

bool CatalogController::removeCategory(const std::string& category) {
    if (category.empty()) {
        notifyError("Category name cannot be empty");
        return false;
    }
    
    if (catalogService_) {
        return catalogService_->removeCategory(category);
    }
    
    return false;
}

bool CatalogController::renameCategory(const std::string& oldName, const std::string& newName) {
    if (oldName.empty() || newName.empty()) {
        notifyError("Category names cannot be empty");
        return false;
    }
    
    if (catalogService_) {
        return catalogService_->renameCategory(oldName, newName);
    }
    
    return false;
}

std::unordered_map<std::string, size_t> CatalogController::getCategoryCounts() const {
    if (catalogService_) {
        return catalogService_->getCategoryCounts();
    }
    
    return {};
}

std::vector<std::string> CatalogController::getStandardCategories() const {
    return CatalogItem::getStandardCategories();
}

CatalogSearchResult CatalogController::searchItems(const std::string& searchTerm, 
                                                  size_t offset, size_t limit) const {
    if (catalogService_) {
        return catalogService_->searchItems(searchTerm, offset, limit);
    }
    
    return CatalogSearchResult();
}

CatalogSearchResult CatalogController::filterItems(const CatalogFilter& filter,
                                                  size_t offset, size_t limit) const {
    if (catalogService_) {
        return catalogService_->filterItems(filter, offset, limit);
    }
    
    return CatalogSearchResult();
}

CatalogSearchResult CatalogController::advancedSearch(const std::string& searchTerm,
                                                     const CatalogFilter& filter,
                                                     size_t offset, size_t limit) const {
    if (catalogService_) {
        return catalogService_->advancedSearch(searchTerm, filter, offset, limit);
    }
    
    return CatalogSearchResult();
}

std::vector<std::shared_ptr<CatalogItem>> CatalogController::getSimilarItems(const std::string& itemId,
                                                                            size_t maxResults) const {
    if (catalogService_) {
        return catalogService_->getSimilarItems(itemId, maxResults);
    }
    
    return {};
}

std::vector<std::shared_ptr<CatalogItem>> CatalogController::getRecommendedItems(const std::string& category,
                                                                                size_t maxResults) const {
    if (catalogService_) {
        return catalogService_->getRecommendedItems(category, maxResults);
    }
    
    return {};
}

CatalogFilter CatalogController::createFilter(const std::vector<std::string>& categories,
                                             const Dimensions3D& minDims,
                                             const Dimensions3D& maxDims,
                                             double minPrice,
                                             double maxPrice,
                                             const std::vector<std::string>& features) const {
    CatalogFilter filter;
    filter.categories = categories;
    filter.minDimensions = minDims;
    filter.maxDimensions = maxDims;
    filter.minPrice = minPrice;
    filter.maxPrice = maxPrice;
    filter.features = features;
    
    return filter;
}

CatalogOperationResult CatalogController::importFromJson(const std::string& filePath, 
                                                        bool includeImages,
                                                        bool includeMaterials,
                                                        bool overwriteExisting) {
    if (!validateFilePath(filePath, ".json")) {
        return CatalogOperationResult(false, "Invalid JSON file path: " + filePath);
    }
    
    try {
        ImportExportOptions options = createImportExportOptions(includeImages, includeMaterials, overwriteExisting);
        bool success = catalogService_->importFromJson(filePath, options);
        
        if (!success) {
            return CatalogOperationResult(false, "Failed to import catalog from JSON");
        }
        
        notifyCatalogLoaded("Catalog imported from JSON: " + filePath);
        return CatalogOperationResult(true, "Catalog imported successfully from JSON");
        
    } catch (const std::exception& e) {
        std::string error = "Failed to import from JSON: " + std::string(e.what());
        notifyError(error);
        return CatalogOperationResult(false, error);
    }
}

CatalogOperationResult CatalogController::exportToJson(const std::string& filePath,
                                                      bool includeImages,
                                                      bool includeMaterials) const {
    if (!validateFilePath(filePath, ".json")) {
        return CatalogOperationResult(false, "Invalid JSON file path: " + filePath);
    }
    
    try {
        ImportExportOptions options = createImportExportOptions(includeImages, includeMaterials);
        bool success = catalogService_->exportToJson(filePath, options);
        
        if (!success) {
            return CatalogOperationResult(false, "Failed to export catalog to JSON");
        }
        
        return CatalogOperationResult(true, "Catalog exported successfully to JSON");
        
    } catch (const std::exception& e) {
        std::string error = "Failed to export to JSON: " + std::string(e.what());
        notifyError(error);
        return CatalogOperationResult(false, error);
    }
}

CatalogOperationResult CatalogController::importItemFromJson(const nlohmann::json& itemJson,
                                                            bool includeImages,
                                                            bool includeMaterials) {
    try {
        ImportExportOptions options = createImportExportOptions(includeImages, includeMaterials);
        auto item = catalogService_->importItemFromJson(itemJson, options);
        
        if (!item) {
            return CatalogOperationResult(false, "Failed to import item from JSON");
        }
        
        notifyItemAdded(item->getId());
        return CatalogOperationResult(true, "Item imported successfully from JSON", item->getId());
        
    } catch (const std::exception& e) {
        std::string error = "Failed to import item from JSON: " + std::string(e.what());
        notifyError(error);
        return CatalogOperationResult(false, error);
    }
}

CatalogOperationResult CatalogController::exportCategoryToJson(const std::string& category, 
                                                              const std::string& filePath,
                                                              bool includeImages,
                                                              bool includeMaterials) const {
    if (category.empty()) {
        return CatalogOperationResult(false, "Category name cannot be empty");
    }
    
    if (!validateFilePath(filePath, ".json")) {
        return CatalogOperationResult(false, "Invalid JSON file path: " + filePath);
    }
    
    try {
        ImportExportOptions options = createImportExportOptions(includeImages, includeMaterials);
        bool success = catalogService_->exportCategoryToJson(category, filePath, options);
        
        if (!success) {
            return CatalogOperationResult(false, "Failed to export category to JSON");
        }
        
        return CatalogOperationResult(true, "Category exported successfully to JSON");
        
    } catch (const std::exception& e) {
        std::string error = "Failed to export category to JSON: " + std::string(e.what());
        notifyError(error);
        return CatalogOperationResult(false, error);
    }
}

CatalogOperationResult CatalogController::importFromCsv(const std::string& filePath,
                                                       bool includeImages,
                                                       bool overwriteExisting) {
    if (!validateFilePath(filePath, ".csv")) {
        return CatalogOperationResult(false, "Invalid CSV file path: " + filePath);
    }
    
    try {
        ImportExportOptions options = createImportExportOptions(includeImages, true, overwriteExisting);
        bool success = catalogService_->importFromCsv(filePath, options);
        
        if (!success) {
            return CatalogOperationResult(false, "Failed to import catalog from CSV");
        }
        
        notifyCatalogLoaded("Catalog imported from CSV: " + filePath);
        return CatalogOperationResult(true, "Catalog imported successfully from CSV");
        
    } catch (const std::exception& e) {
        std::string error = "Failed to import from CSV: " + std::string(e.what());
        notifyError(error);
        return CatalogOperationResult(false, error);
    }
}

CatalogOperationResult CatalogController::exportToCsv(const std::string& filePath) const {
    if (!validateFilePath(filePath, ".csv")) {
        return CatalogOperationResult(false, "Invalid CSV file path: " + filePath);
    }
    
    try {
        bool success = catalogService_->exportToCsv(filePath);
        
        if (!success) {
            return CatalogOperationResult(false, "Failed to export catalog to CSV");
        }
        
        return CatalogOperationResult(true, "Catalog exported successfully to CSV");
        
    } catch (const std::exception& e) {
        std::string error = "Failed to export to CSV: " + std::string(e.what());
        notifyError(error);
        return CatalogOperationResult(false, error);
    }
}

std::vector<std::string> CatalogController::validateCatalog() const {
    if (catalogService_) {
        return catalogService_->validateCatalog();
    }
    
    return {"Catalog service not available"};
}

std::vector<std::string> CatalogController::validateItem(const std::string& itemId) const {
    if (catalogService_) {
        return catalogService_->validateItem(itemId);
    }
    
    return {"Catalog service not available"};
}

std::vector<std::pair<std::string, std::string>> CatalogController::findDuplicates() const {
    if (catalogService_) {
        return catalogService_->findDuplicates();
    }
    
    return {};
}

std::vector<std::string> CatalogController::cleanupOrphanedFiles() const {
    if (catalogService_) {
        return catalogService_->cleanupOrphanedFiles();
    }
    
    return {};
}

void CatalogController::rebuildSearchIndex() {
    if (catalogService_) {
        catalogService_->rebuildSearchIndex();
    }
}

void CatalogController::optimizeCatalog() {
    if (catalogService_) {
        catalogService_->optimize();
    }
}

CatalogService::CatalogStatistics CatalogController::getStatistics() const {
    if (catalogService_) {
        return catalogService_->getStatistics();
    }
    
    return CatalogService::CatalogStatistics();
}

std::unordered_map<std::string, std::pair<double, double>> CatalogController::getPriceRangesByCategory() const {
    if (catalogService_) {
        return catalogService_->getPriceRangesByCategory();
    }
    
    return {};
}

std::vector<std::shared_ptr<CatalogItem>> CatalogController::getMostPopularItems(size_t maxResults) const {
    if (catalogService_) {
        return catalogService_->getMostPopularItems(maxResults);
    }
    
    return {};
}

std::vector<std::shared_ptr<CatalogItem>> CatalogController::getRecentlyAddedItems(size_t maxResults) const {
    if (catalogService_) {
        return catalogService_->getRecentlyAddedItems(maxResults);
    }
    
    return {};
}

void CatalogController::setCatalogBasePath(const std::string& path) {
    if (catalogService_) {
        catalogService_->setCatalogBasePath(path);
    }
}

std::string CatalogController::getCatalogBasePath() const {
    if (catalogService_) {
        return catalogService_->getCatalogBasePath();
    }
    
    return "";
}

void CatalogController::setDefaultImportOptions(bool includeImages, bool includeMaterials, bool overwriteExisting) {
    if (catalogService_) {
        ImportExportOptions options = createImportExportOptions(includeImages, includeMaterials, overwriteExisting);
        catalogService_->setDefaultImportOptions(options);
    }
}

ImportExportOptions CatalogController::getDefaultImportOptions() const {
    if (catalogService_) {
        return catalogService_->getDefaultImportOptions();
    }
    
    return ImportExportOptions();
}

CatalogOperationResult CatalogController::addItems(const std::vector<std::shared_ptr<CatalogItem>>& items) {
    if (items.empty()) {
        return CatalogOperationResult(false, "No items to add");
    }
    
    try {
        size_t addedCount = catalogService_->addItems(items);
        
        if (addedCount == 0) {
            return CatalogOperationResult(false, "Failed to add any items");
        }
        
        for (const auto& item : items) {
            if (item) {
                notifyItemAdded(item->getId());
            }
        }
        
        std::string message = "Added " + std::to_string(addedCount) + " items successfully";
        return CatalogOperationResult(true, message);
        
    } catch (const std::exception& e) {
        std::string error = "Failed to add items: " + std::string(e.what());
        notifyError(error);
        return CatalogOperationResult(false, error);
    }
}

CatalogOperationResult CatalogController::removeItems(const std::vector<std::string>& itemIds) {
    if (itemIds.empty()) {
        return CatalogOperationResult(false, "No items to remove");
    }
    
    try {
        size_t removedCount = catalogService_->removeItems(itemIds);
        
        if (removedCount == 0) {
            return CatalogOperationResult(false, "Failed to remove any items");
        }
        
        for (const auto& itemId : itemIds) {
            notifyItemRemoved(itemId);
        }
        
        std::string message = "Removed " + std::to_string(removedCount) + " items successfully";
        return CatalogOperationResult(true, message);
        
    } catch (const std::exception& e) {
        std::string error = "Failed to remove items: " + std::string(e.what());
        notifyError(error);
        return CatalogOperationResult(false, error);
    }
}

CatalogOperationResult CatalogController::updateItems(const std::vector<std::shared_ptr<CatalogItem>>& items) {
    if (items.empty()) {
        return CatalogOperationResult(false, "No items to update");
    }
    
    try {
        size_t updatedCount = catalogService_->updateItems(items);
        
        if (updatedCount == 0) {
            return CatalogOperationResult(false, "Failed to update any items");
        }
        
        for (const auto& item : items) {
            if (item) {
                notifyItemUpdated(item->getId());
            }
        }
        
        std::string message = "Updated " + std::to_string(updatedCount) + " items successfully";
        return CatalogOperationResult(true, message);
        
    } catch (const std::exception& e) {
        std::string error = "Failed to update items: " + std::string(e.what());
        notifyError(error);
        return CatalogOperationResult(false, error);
    }
}

CatalogOperationResult CatalogController::clearCatalog() {
    try {
        if (catalogService_) {
            catalogService_->clear();
            notifyCatalogLoaded("Catalog cleared");
            return CatalogOperationResult(true, "Catalog cleared successfully");
        }
        
        return CatalogOperationResult(false, "Catalog service not available");
        
    } catch (const std::exception& e) {
        std::string error = "Failed to clear catalog: " + std::string(e.what());
        notifyError(error);
        return CatalogOperationResult(false, error);
    }
}

std::shared_ptr<CatalogItem> CatalogController::createCabinetItem(const std::string& id, const std::string& name,
                                                                 const std::string& subCategory,
                                                                 const Dimensions3D& dimensions,
                                                                 double basePrice) const {
    auto item = std::make_shared<CatalogItem>(id, name, "cabinets");
    item->setDimensions(dimensions);
    item->setBasePrice(basePrice);
    
    Specifications specs = createDefaultSpecifications("cabinets");
    specs.installationType = subCategory;
    item->setSpecifications(specs);
    
    return item;
}

std::shared_ptr<CatalogItem> CatalogController::createApplianceItem(const std::string& id, const std::string& name,
                                                                   const std::string& brand,
                                                                   const Dimensions3D& dimensions,
                                                                   double basePrice) const {
    auto item = std::make_shared<CatalogItem>(id, name, "appliances");
    item->setDimensions(dimensions);
    item->setBasePrice(basePrice);
    
    Specifications specs = createDefaultSpecifications("appliances");
    specs.additionalInfo = "{\"brand\":\"" + brand + "\"}";
    item->setSpecifications(specs);
    
    return item;
}

std::shared_ptr<CatalogItem> CatalogController::createCountertopItem(const std::string& id, const std::string& name,
                                                                    const std::string& material,
                                                                    const Dimensions3D& dimensions,
                                                                    double pricePerSquareMeter) const {
    auto item = std::make_shared<CatalogItem>(id, name, "countertops");
    item->setDimensions(dimensions);
    
    // Calculate price based on area
    double area = dimensions.width * dimensions.depth;
    item->setBasePrice(area * pricePerSquareMeter);
    
    Specifications specs = createDefaultSpecifications("countertops");
    specs.material = material;
    item->setSpecifications(specs);
    
    return item;
}

std::shared_ptr<CatalogItem> CatalogController::createHardwareItem(const std::string& id, const std::string& name,
                                                                  const std::string& type,
                                                                  double basePrice) const {
    auto item = std::make_shared<CatalogItem>(id, name, "hardware");
    item->setDimensions(Dimensions3D(0.05, 0.05, 0.02)); // Default small dimensions
    item->setBasePrice(basePrice);
    
    Specifications specs = createDefaultSpecifications("hardware");
    specs.hardware = type;
    item->setSpecifications(specs);
    
    return item;
}

std::string CatalogController::generateUniqueItemId(const std::string& category) const {
    if (catalogService_) {
        return catalogService_->generateItemId(category);
    }
    
    return CatalogItem::generateId();
}

bool CatalogController::isValidItemId(const std::string& itemId) {
    return CatalogService::isValidItemId(itemId);
}

std::string CatalogController::normalizeCategory(const std::string& category) {
    return CatalogService::normalizeCategory(category);
}

bool CatalogController::isValidItemData(const std::string& name, const std::string& category, 
                                       const Dimensions3D& dimensions, double basePrice) const {
    if (name.empty() || name.length() > 255) {
        return false;
    }
    
    if (category.empty() || category.length() > 100) {
        return false;
    }
    
    if (!dimensions.isValid()) {
        return false;
    }
    
    if (basePrice < 0.0) {
        return false;
    }
    
    return true;
}

void CatalogController::setupCallbacks() {
    if (!catalogService_) {
        return;
    }
    
    catalogService_->setItemAddedCallback([this](const std::string& itemId) {
        notifyItemAdded(itemId);
    });
    
    catalogService_->setItemRemovedCallback([this](const std::string& itemId) {
        notifyItemRemoved(itemId);
    });
    
    catalogService_->setItemUpdatedCallback([this](const std::string& itemId) {
        notifyItemUpdated(itemId);
    });
    
    catalogService_->setErrorCallback([this](const std::string& error) {
        notifyError(error);
    });
}

void CatalogController::notifyItemAdded(const std::string& itemId) {
    if (itemAddedCallback_) {
        itemAddedCallback_(itemId);
    }
}

void CatalogController::notifyItemRemoved(const std::string& itemId) {
    if (itemRemovedCallback_) {
        itemRemovedCallback_(itemId);
    }
}

void CatalogController::notifyItemUpdated(const std::string& itemId) {
    if (itemUpdatedCallback_) {
        itemUpdatedCallback_(itemId);
    }
}

void CatalogController::notifyCatalogLoaded(const std::string& message) {
    if (catalogLoadedCallback_) {
        catalogLoadedCallback_(message);
    }
}

void CatalogController::notifyError(const std::string& error) const {
    if (errorCallback_) {
        errorCallback_(error);
    }
}

ImportExportOptions CatalogController::createImportExportOptions(bool includeImages, bool includeMaterials, 
                                                                bool overwriteExisting) const {
    ImportExportOptions options;
    options.includeImages = includeImages;
    options.includeMaterials = includeMaterials;
    options.overwriteExisting = overwriteExisting;
    
    return options;
}

bool CatalogController::validateFilePath(const std::string& filePath, const std::string& expectedExtension) const {
    if (filePath.empty()) {
        return false;
    }
    
    std::filesystem::path path(filePath);
    
    // Check if extension matches
    if (path.extension() != expectedExtension) {
        return false;
    }
    
    // Check if parent directory exists or can be created
    auto parentPath = path.parent_path();
    if (!parentPath.empty() && !std::filesystem::exists(parentPath)) {
        std::error_code ec;
        std::filesystem::create_directories(parentPath, ec);
        if (ec) {
            return false;
        }
    }
    
    return true;
}

Specifications CatalogController::createDefaultSpecifications(const std::string& category) const {
    Specifications specs;
    
    if (category == "cabinets") {
        specs.material = "Plywood";
        specs.finish = "Laminate";
        specs.hardware = "Standard hinges";
        specs.loadCapacity = 50.0; // kg
        specs.installationType = "Wall mounted";
        specs.features = {"Adjustable shelves", "Soft close doors"};
    } else if (category == "appliances") {
        specs.material = "Stainless steel";
        specs.finish = "Brushed";
        specs.installationType = "Built-in";
        specs.features = {"Energy efficient", "Digital controls"};
    } else if (category == "countertops") {
        specs.material = "Quartz";
        specs.finish = "Polished";
        specs.loadCapacity = 100.0; // kg/m²
        specs.installationType = "Adhesive mounted";
        specs.features = {"Stain resistant", "Heat resistant"};
    } else if (category == "hardware") {
        specs.material = "Stainless steel";
        specs.finish = "Brushed";
        specs.installationType = "Screw mounted";
        specs.features = {"Corrosion resistant"};
    }
    
    return specs;
}

} // namespace Controllers
} // namespace KitchenCAD