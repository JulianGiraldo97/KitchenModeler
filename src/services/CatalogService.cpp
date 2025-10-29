#include "CatalogService.h"
#include "../utils/Logger.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <regex>

namespace KitchenCAD {
namespace Services {

CatalogService::CatalogService(const std::string& catalogBasePath)
    : catalogBasePath_(catalogBasePath)
{
    initializeStandardCategories();
    
    // Create catalog directory structure
    std::filesystem::create_directories(catalogBasePath_);
    std::filesystem::create_directories(catalogBasePath_ + "/images");
    std::filesystem::create_directories(catalogBasePath_ + "/models");
}

bool CatalogService::addItem(std::shared_ptr<CatalogItem> item) {
    if (!item || !item->isValid()) {
        notifyError("Invalid catalog item");
        return false;
    }
    
    const std::string& itemId = item->getId();
    
    // Check for duplicate ID
    if (hasItem(itemId)) {
        notifyError("Item with ID already exists: " + itemId);
        return false;
    }
    
    // Validate file paths
    if (!validateItemPaths(*item)) {
        notifyError("Invalid file paths in item: " + itemId);
        return false;
    }
    
    // Add to main collection
    items_[itemId] = item;
    
    // Update indices
    updateCategoryIndex(itemId, item->getCategory(), true);
    updateSearchIndex(item, true);
    
    notifyItemAdded(itemId);
    return true;
}

bool CatalogService::removeItem(const std::string& itemId) {
    auto it = items_.find(itemId);
    if (it == items_.end()) {
        notifyError("Item not found: " + itemId);
        return false;
    }
    
    auto item = it->second;
    
    // Update indices
    updateCategoryIndex(itemId, item->getCategory(), false);
    updateSearchIndex(item, false);
    
    // Remove from main collection
    items_.erase(it);
    
    notifyItemRemoved(itemId);
    return true;
}

bool CatalogService::updateItem(std::shared_ptr<CatalogItem> item) {
    if (!item || !item->isValid()) {
        notifyError("Invalid catalog item for update");
        return false;
    }
    
    const std::string& itemId = item->getId();
    
    auto it = items_.find(itemId);
    if (it == items_.end()) {
        notifyError("Item not found for update: " + itemId);
        return false;
    }
    
    auto oldItem = it->second;
    
    // Update indices if category changed
    if (oldItem->getCategory() != item->getCategory()) {
        updateCategoryIndex(itemId, oldItem->getCategory(), false);
        updateCategoryIndex(itemId, item->getCategory(), true);
    }
    
    // Update search index
    updateSearchIndex(oldItem, false);
    updateSearchIndex(item, true);
    
    // Update item
    items_[itemId] = item;
    
    notifyItemUpdated(itemId);
    return true;
}

std::shared_ptr<CatalogItem> CatalogService::getItem(const std::string& itemId) const {
    auto it = items_.find(itemId);
    return (it != items_.end()) ? it->second : nullptr;
}

bool CatalogService::hasItem(const std::string& itemId) const {
    return items_.find(itemId) != items_.end();
}

std::vector<std::shared_ptr<CatalogItem>> CatalogService::getAllItems() const {
    std::vector<std::shared_ptr<CatalogItem>> result;
    result.reserve(items_.size());
    
    for (const auto& pair : items_) {
        result.push_back(pair.second);
    }
    
    return result;
}

std::vector<std::string> CatalogService::getCategories() const {
    return std::vector<std::string>(categories_.begin(), categories_.end());
}

std::vector<std::shared_ptr<CatalogItem>> CatalogService::getItemsByCategory(const std::string& category) const {
    std::vector<std::shared_ptr<CatalogItem>> result;
    
    auto it = categorizedItems_.find(category);
    if (it != categorizedItems_.end()) {
        result.reserve(it->second.size());
        
        for (const std::string& itemId : it->second) {
            auto item = getItem(itemId);
            if (item) {
                result.push_back(item);
            }
        }
    }
    
    return result;
}

void CatalogService::addCategory(const std::string& category) {
    std::string normalizedCategory = normalizeCategory(category);
    categories_.insert(normalizedCategory);
}

bool CatalogService::removeCategory(const std::string& category) {
    auto it = categorizedItems_.find(category);
    if (it == categorizedItems_.end()) {
        return false;
    }
    
    // Move items to "uncategorized"
    const std::string uncategorized = "uncategorized";
    addCategory(uncategorized);
    
    for (const std::string& itemId : it->second) {
        auto item = getItem(itemId);
        if (item) {
            item->setCategory(uncategorized);
            updateCategoryIndex(itemId, uncategorized, true);
        }
    }
    
    categorizedItems_.erase(it);
    categories_.erase(category);
    
    return true;
}

bool CatalogService::renameCategory(const std::string& oldName, const std::string& newName) {
    auto it = categorizedItems_.find(oldName);
    if (it == categorizedItems_.end()) {
        return false;
    }
    
    std::string normalizedNewName = normalizeCategory(newName);
    
    // Update all items in the category
    for (const std::string& itemId : it->second) {
        auto item = getItem(itemId);
        if (item) {
            item->setCategory(normalizedNewName);
        }
    }
    
    // Move category data
    categorizedItems_[normalizedNewName] = std::move(it->second);
    categorizedItems_.erase(it);
    
    // Update category set
    categories_.erase(oldName);
    categories_.insert(normalizedNewName);
    
    return true;
}

std::unordered_map<std::string, size_t> CatalogService::getCategoryCounts() const {
    std::unordered_map<std::string, size_t> counts;
    
    for (const auto& pair : categorizedItems_) {
        counts[pair.first] = pair.second.size();
    }
    
    return counts;
}

CatalogSearchResult CatalogService::searchItems(const std::string& searchTerm, 
                                               size_t offset, size_t limit) const {
    std::vector<std::shared_ptr<CatalogItem>> results;
    
    if (searchTerm.empty()) {
        // Return all items if no search term
        results = getAllItems();
    } else {
        std::string normalizedTerm = normalizeSearchTerm(searchTerm);
        std::unordered_set<std::string> matchingItems;
        
        // Search in index
        auto it = searchIndex_.find(normalizedTerm);
        if (it != searchIndex_.end()) {
            matchingItems = it->second;
        } else {
            // Partial matching
            for (const auto& indexPair : searchIndex_) {
                if (indexPair.first.find(normalizedTerm) != std::string::npos) {
                    matchingItems.insert(indexPair.second.begin(), indexPair.second.end());
                }
            }
        }
        
        // Convert to items
        for (const std::string& itemId : matchingItems) {
            auto item = getItem(itemId);
            if (item) {
                results.push_back(item);
            }
        }
    }
    
    // Apply pagination
    size_t totalCount = results.size();
    if (offset >= totalCount) {
        return CatalogSearchResult({}, totalCount, offset, limit);
    }
    
    size_t endIndex = std::min(offset + limit, totalCount);
    std::vector<std::shared_ptr<CatalogItem>> paginatedResults(
        results.begin() + offset, results.begin() + endIndex);
    
    return CatalogSearchResult(paginatedResults, totalCount, offset, limit);
}

CatalogSearchResult CatalogService::filterItems(const CatalogFilter& filter,
                                               size_t offset, size_t limit) const {
    std::vector<std::shared_ptr<CatalogItem>> results;
    
    for (const auto& pair : items_) {
        if (filter.matches(*pair.second)) {
            results.push_back(pair.second);
        }
    }
    
    // Apply pagination
    size_t totalCount = results.size();
    if (offset >= totalCount) {
        return CatalogSearchResult({}, totalCount, offset, limit);
    }
    
    size_t endIndex = std::min(offset + limit, totalCount);
    std::vector<std::shared_ptr<CatalogItem>> paginatedResults(
        results.begin() + offset, results.begin() + endIndex);
    
    return CatalogSearchResult(paginatedResults, totalCount, offset, limit);
}

CatalogSearchResult CatalogService::advancedSearch(const std::string& searchTerm,
                                                  const CatalogFilter& filter,
                                                  size_t offset, size_t limit) const {
    // First apply text search
    auto textResults = searchItems(searchTerm, 0, SIZE_MAX);
    
    // Then apply filter to text results
    std::vector<std::shared_ptr<CatalogItem>> results;
    for (const auto& item : textResults.items) {
        if (filter.matches(*item)) {
            results.push_back(item);
        }
    }
    
    // Apply pagination
    size_t totalCount = results.size();
    if (offset >= totalCount) {
        return CatalogSearchResult({}, totalCount, offset, limit);
    }
    
    size_t endIndex = std::min(offset + limit, totalCount);
    std::vector<std::shared_ptr<CatalogItem>> paginatedResults(
        results.begin() + offset, results.begin() + endIndex);
    
    return CatalogSearchResult(paginatedResults, totalCount, offset, limit);
}

std::vector<std::shared_ptr<CatalogItem>> CatalogService::getSimilarItems(const std::string& itemId,
                                                                         size_t maxResults) const {
    auto targetItem = getItem(itemId);
    if (!targetItem) {
        return {};
    }
    
    std::vector<std::pair<std::shared_ptr<CatalogItem>, double>> scoredItems;
    
    for (const auto& pair : items_) {
        if (pair.first == itemId) continue; // Skip self
        
        auto item = pair.second;
        double score = 0.0;
        
        // Category match (high weight)
        if (item->getCategory() == targetItem->getCategory()) {
            score += 50.0;
        }
        
        // Dimension similarity (medium weight)
        auto targetDims = targetItem->getDimensions();
        auto itemDims = item->getDimensions();
        
        double dimScore = 0.0;
        if (targetDims.isValid() && itemDims.isValid()) {
            double widthDiff = std::abs(targetDims.width - itemDims.width) / std::max(targetDims.width, itemDims.width);
            double heightDiff = std::abs(targetDims.height - itemDims.height) / std::max(targetDims.height, itemDims.height);
            double depthDiff = std::abs(targetDims.depth - itemDims.depth) / std::max(targetDims.depth, itemDims.depth);
            
            dimScore = 30.0 * (1.0 - (widthDiff + heightDiff + depthDiff) / 3.0);
        }
        score += std::max(0.0, dimScore);
        
        // Price similarity (low weight)
        double targetPrice = targetItem->getBasePrice();
        double itemPrice = item->getBasePrice();
        if (targetPrice > 0 && itemPrice > 0) {
            double priceDiff = std::abs(targetPrice - itemPrice) / std::max(targetPrice, itemPrice);
            score += 20.0 * (1.0 - priceDiff);
        }
        
        if (score > 0) {
            scoredItems.emplace_back(item, score);
        }
    }
    
    // Sort by score (descending)
    std::sort(scoredItems.begin(), scoredItems.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Extract top results
    std::vector<std::shared_ptr<CatalogItem>> results;
    size_t count = std::min(maxResults, scoredItems.size());
    results.reserve(count);
    
    for (size_t i = 0; i < count; ++i) {
        results.push_back(scoredItems[i].first);
    }
    
    return results;
}

std::vector<std::shared_ptr<CatalogItem>> CatalogService::getRecommendedItems(const std::string& category,
                                                                             size_t maxResults) const {
    std::vector<std::shared_ptr<CatalogItem>> results;
    
    if (category.empty()) {
        // Return recently added items across all categories
        results = getRecentlyAddedItems(maxResults);
    } else {
        // Return items from specified category
        results = getItemsByCategory(category);
        
        // Sort by some criteria (e.g., price, recently added)
        std::sort(results.begin(), results.end(),
            [](const auto& a, const auto& b) {
                return a->getUpdatedAt() > b->getUpdatedAt();
            });
        
        // Limit results
        if (results.size() > maxResults) {
            results.resize(maxResults);
        }
    }
    
    return results;
}

bool CatalogService::importFromJson(const std::string& filePath, const ImportExportOptions& options) {
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            notifyError("Cannot open file for import: " + filePath);
            return false;
        }
        
        nlohmann::json catalogJson;
        file >> catalogJson;
        
        if (!catalogJson.is_object() || !catalogJson.contains("items")) {
            notifyError("Invalid catalog JSON format");
            return false;
        }
        
        size_t importedCount = 0;
        size_t skippedCount = 0;
        
        for (const auto& itemJson : catalogJson["items"]) {
            auto item = importItemFromJson(itemJson, options);
            if (item) {
                if (hasItem(item->getId()) && !options.overwriteExisting) {
                    skippedCount++;
                    continue;
                }
                
                if (addItem(item) || (options.overwriteExisting && updateItem(item))) {
                    importedCount++;
                } else {
                    skippedCount++;
                }
            } else {
                skippedCount++;
            }
        }
        
        notifyItemAdded("Imported " + std::to_string(importedCount) + " items, skipped " + std::to_string(skippedCount));
        return importedCount > 0;
        
    } catch (const std::exception& e) {
        notifyError("Exception during JSON import: " + std::string(e.what()));
        return false;
    }
}

bool CatalogService::exportToJson(const std::string& filePath, const ImportExportOptions& options) const {
    try {
        nlohmann::json catalogJson;
        catalogJson["version"] = "1.0";
        catalogJson["exported_at"] = std::chrono::system_clock::now().time_since_epoch().count();
        catalogJson["total_items"] = items_.size();
        
        nlohmann::json itemsArray = nlohmann::json::array();
        
        for (const auto& pair : items_) {
            auto itemJson = pair.second->toJson();
            
            // Optionally exclude images/models from export
            if (!options.includeImages) {
                itemJson.erase("thumbnailPath");
            }
            if (!options.includeMaterials) {
                itemJson.erase("materialOptions");
            }
            
            itemsArray.push_back(itemJson);
        }
        
        catalogJson["items"] = itemsArray;
        
        std::ofstream file(filePath);
        if (!file.is_open()) {
            notifyError("Cannot open file for export: " + filePath);
            return false;
        }
        
        file << catalogJson.dump(2);
        return true;
        
    } catch (const std::exception& e) {
        notifyError("Exception during JSON export: " + std::string(e.what()));
        return false;
    }
}

std::shared_ptr<CatalogItem> CatalogService::importItemFromJson(const nlohmann::json& itemJson,
                                                               const ImportExportOptions& options) {
    try {
        auto item = std::make_shared<CatalogItem>();
        item->fromJson(itemJson);
        
        // Validate imported item
        if (!item->isValid()) {
            notifyError("Invalid item data in JSON");
            return nullptr;
        }
        
        // Adjust file paths if needed
        if (options.includeImages && !item->getThumbnailPath().empty()) {
            std::filesystem::path thumbnailPath = item->getThumbnailPath();
            if (thumbnailPath.is_relative()) {
                item->setThumbnailPath((std::filesystem::path(options.imageBasePath) / thumbnailPath).string());
            }
        }
        
        if (!item->getModelPath().empty()) {
            std::filesystem::path modelPath = item->getModelPath();
            if (modelPath.is_relative()) {
                item->setModelPath((std::filesystem::path(options.modelBasePath) / modelPath).string());
            }
        }
        
        return item;
        
    } catch (const std::exception& e) {
        notifyError("Exception importing item from JSON: " + std::string(e.what()));
        return nullptr;
    }
}

CatalogService::CatalogStatistics CatalogService::getStatistics() const {
    CatalogStatistics stats;
    stats.totalItems = items_.size();
    
    double totalPrice = 0.0;
    stats.minPrice = std::numeric_limits<double>::max();
    stats.maxPrice = 0.0;
    
    for (const auto& pair : items_) {
        const auto& item = pair.second;
        
        // Category count
        stats.itemsByCategory[item->getCategory()]++;
        
        // Price statistics
        double price = item->getBasePrice();
        if (price > 0) {
            totalPrice += price;
            stats.minPrice = std::min(stats.minPrice, price);
            stats.maxPrice = std::max(stats.maxPrice, price);
        }
        
        // File statistics
        if (!item->getThumbnailPath().empty()) {
            stats.itemsWithImages++;
        }
        if (!item->getModelPath().empty()) {
            stats.itemsWithModels++;
        }
    }
    
    if (stats.totalItems > 0) {
        stats.averagePrice = totalPrice / stats.totalItems;
    }
    
    if (stats.minPrice == std::numeric_limits<double>::max()) {
        stats.minPrice = 0.0;
    }
    
    return stats;
}

std::vector<std::shared_ptr<CatalogItem>> CatalogService::getRecentlyAddedItems(size_t maxResults) const {
    std::vector<std::shared_ptr<CatalogItem>> allItems = getAllItems();
    
    // Sort by creation time (newest first)
    std::sort(allItems.begin(), allItems.end(),
        [](const auto& a, const auto& b) {
            return a->getCreatedAt() > b->getCreatedAt();
        });
    
    // Limit results
    if (allItems.size() > maxResults) {
        allItems.resize(maxResults);
    }
    
    return allItems;
}

void CatalogService::rebuildSearchIndex() {
    searchIndex_.clear();
    
    for (const auto& pair : items_) {
        updateSearchIndex(pair.second, true);
    }
}

std::string CatalogService::generateItemId(const std::string& category) const {
    std::string prefix = category.empty() ? "item" : normalizeCategory(category);
    
    // Find next available number
    int counter = 1;
    std::string candidateId;
    
    do {
        candidateId = prefix + "_" + std::to_string(counter);
        counter++;
    } while (hasItem(candidateId));
    
    return candidateId;
}

bool CatalogService::isValidItemId(const std::string& itemId) {
    // Simple validation: alphanumeric and underscores only
    std::regex idPattern("^[a-zA-Z0-9_]+$");
    return std::regex_match(itemId, idPattern);
}

std::string CatalogService::normalizeCategory(const std::string& category) {
    std::string normalized = category;
    
    // Convert to lowercase
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    
    // Replace spaces with underscores
    std::replace(normalized.begin(), normalized.end(), ' ', '_');
    
    // Remove special characters
    normalized.erase(std::remove_if(normalized.begin(), normalized.end(),
        [](char c) { return !std::isalnum(c) && c != '_'; }), normalized.end());
    
    return normalized;
}

std::vector<std::string> CatalogService::getStandardCategories() {
    return {
        "base_cabinets",
        "wall_cabinets",
        "tall_cabinets",
        "corner_cabinets",
        "appliances",
        "countertops",
        "sinks",
        "faucets",
        "hardware",
        "accessories",
        "lighting",
        "flooring"
    };
}

void CatalogService::updateCategoryIndex(const std::string& itemId, const std::string& category, bool add) {
    if (add) {
        categorizedItems_[category].push_back(itemId);
        categories_.insert(category);
    } else {
        auto& categoryItems = categorizedItems_[category];
        categoryItems.erase(std::remove(categoryItems.begin(), categoryItems.end(), itemId), categoryItems.end());
        
        // Remove category if empty
        if (categoryItems.empty()) {
            categorizedItems_.erase(category);
            // Don't remove from categories_ set to keep standard categories
        }
    }
}

void CatalogService::updateSearchIndex(const std::shared_ptr<CatalogItem>& item, bool add) {
    auto searchTerms = extractSearchTerms(*item);
    
    for (const std::string& term : searchTerms) {
        std::string normalizedTerm = normalizeSearchTerm(term);
        
        if (add) {
            searchIndex_[normalizedTerm].insert(item->getId());
        } else {
            auto it = searchIndex_.find(normalizedTerm);
            if (it != searchIndex_.end()) {
                it->second.erase(item->getId());
                if (it->second.empty()) {
                    searchIndex_.erase(it);
                }
            }
        }
    }
}

std::vector<std::string> CatalogService::extractSearchTerms(const CatalogItem& item) const {
    std::vector<std::string> terms;
    
    // Add name words
    std::istringstream nameStream(item.getName());
    std::string word;
    while (nameStream >> word) {
        terms.push_back(word);
    }
    
    // Add category
    terms.push_back(item.getCategory());
    
    // Add ID
    terms.push_back(item.getId());
    
    // Add specifications if available
    // TODO: Extract terms from specifications
    
    return terms;
}

std::string CatalogService::normalizeSearchTerm(const std::string& term) const {
    std::string normalized = term;
    
    // Convert to lowercase
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    
    // Remove punctuation
    normalized.erase(std::remove_if(normalized.begin(), normalized.end(), ::ispunct), normalized.end());
    
    return normalized;
}

void CatalogService::notifyItemAdded(const std::string& itemId) {
    if (itemAddedCallback_) {
        itemAddedCallback_(itemId);
    }
}

void CatalogService::notifyItemRemoved(const std::string& itemId) {
    if (itemRemovedCallback_) {
        itemRemovedCallback_(itemId);
    }
}

void CatalogService::notifyItemUpdated(const std::string& itemId) {
    if (itemUpdatedCallback_) {
        itemUpdatedCallback_(itemId);
    }
}

void CatalogService::notifyError(const std::string& error) const {
    if (errorCallback_) {
        errorCallback_(error);
    }
}

void CatalogService::initializeStandardCategories() {
    auto standardCategories = getStandardCategories();
    for (const std::string& category : standardCategories) {
        categories_.insert(category);
    }
}

bool CatalogService::validateItemPaths(const CatalogItem& item) const {
    // Check thumbnail path
    if (!item.getThumbnailPath().empty()) {
        std::filesystem::path thumbnailPath = item.getThumbnailPath();
        if (thumbnailPath.is_relative()) {
            thumbnailPath = std::filesystem::path(catalogBasePath_) / thumbnailPath;
        }
        if (!std::filesystem::exists(thumbnailPath)) {
            return false;
        }
    }
    
    // Check model path
    if (!item.getModelPath().empty()) {
        std::filesystem::path modelPath = item.getModelPath();
        if (modelPath.is_relative()) {
            modelPath = std::filesystem::path(catalogBasePath_) / modelPath;
        }
        if (!std::filesystem::exists(modelPath)) {
            return false;
        }
    }
    
    return true;
}

} // namespace Services
} // namespace KitchenCAD