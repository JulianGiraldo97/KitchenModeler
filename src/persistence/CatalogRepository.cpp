#include "CatalogRepository.h"
#include "../utils/Logger.h"
#include <nlohmann/json.hpp>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace KitchenCAD {
namespace Persistence {

using json = nlohmann::json;

SQLiteCatalogRepository::SQLiteCatalogRepository(const std::string& databasePath) {
    db_ = std::make_unique<DatabaseManager>(databasePath);
}

SQLiteCatalogRepository::SQLiteCatalogRepository(std::unique_ptr<DatabaseManager> db)
    : db_(std::move(db)) {
}

bool SQLiteCatalogRepository::addItem(const Models::CatalogItem& item) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!db_ || !db_->isOpen()) {
        LOG_ERROR("Database is not open");
        return false;
    }
    
    if (!item.isValid()) {
        LOG_ERROR("Invalid catalog item: " + item.getId());
        return false;
    }
    
    Transaction transaction(*db_);
    
    if (!insertCatalogItem(item)) {
        LOG_ERROR("Failed to insert catalog item: " + item.getId());
        return false;
    }
    
    // Insert material options
    for (const auto& option : item.getMaterialOptions()) {
        if (!addMaterialOption(item.getId(), option)) {
            LOG_ERROR("Failed to insert material option: " + option.id);
            return false;
        }
    }
    
    if (!transaction.commit()) {
        LOG_ERROR("Failed to commit catalog item insertion");
        return false;
    }
    
    LOG_INFO("Added catalog item: " + item.getName() + " (ID: " + item.getId() + ")");
    return true;
}

std::optional<Models::CatalogItem> SQLiteCatalogRepository::getItem(const std::string& itemId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!db_ || !db_->isOpen()) {
        LOG_ERROR("Database is not open");
        return std::nullopt;
    }
    
    auto item = loadCatalogItemFromDatabase(itemId);
    if (!item) {
        return std::nullopt;
    }
    
    if (!loadMaterialOptionsForItem(*item)) {
        LOG_WARNING("Failed to load material options for item: " + itemId);
    }
    
    return item;
}

bool SQLiteCatalogRepository::updateItem(const Models::CatalogItem& item) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!db_ || !db_->isOpen()) {
        LOG_ERROR("Database is not open");
        return false;
    }
    
    if (!item.isValid()) {
        LOG_ERROR("Invalid catalog item: " + item.getId());
        return false;
    }
    
    Transaction transaction(*db_);
    
    if (!updateCatalogItem(item)) {
        LOG_ERROR("Failed to update catalog item: " + item.getId());
        return false;
    }
    
    // Delete existing material options and re-insert
    auto deleteStmt = db_->prepare("DELETE FROM material_options WHERE catalog_item_id = ?");
    if (deleteStmt && deleteStmt->isValid()) {
        deleteStmt->bindText(1, item.getId());
        deleteStmt->step();
    }
    
    // Insert updated material options
    for (const auto& option : item.getMaterialOptions()) {
        if (!addMaterialOption(item.getId(), option)) {
            LOG_ERROR("Failed to update material option: " + option.id);
            return false;
        }
    }
    
    if (!transaction.commit()) {
        LOG_ERROR("Failed to commit catalog item update");
        return false;
    }
    
    LOG_INFO("Updated catalog item: " + item.getName() + " (ID: " + item.getId() + ")");
    return true;
}

bool SQLiteCatalogRepository::deleteItem(const std::string& itemId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!db_ || !db_->isOpen()) {
        LOG_ERROR("Database is not open");
        return false;
    }
    
    auto stmt = db_->prepare("DELETE FROM catalog_items WHERE id = ?");
    if (!stmt || !stmt->isValid()) {
        LOG_ERROR("Failed to prepare delete catalog item statement");
        return false;
    }
    
    stmt->bindText(1, itemId);
    if (!stmt->step()) {
        LOG_ERROR("Failed to delete catalog item: " + itemId);
        return false;
    }
    
    if (db_->getChanges() == 0) {
        LOG_WARNING("Catalog item not found for deletion: " + itemId);
        return false;
    }
    
    LOG_INFO("Deleted catalog item: " + itemId);
    return true;
}

std::vector<Models::CatalogItem> SQLiteCatalogRepository::getAllItems() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<Models::CatalogItem> items;
    
    if (!db_ || !db_->isOpen()) {
        LOG_ERROR("Database is not open");
        return items;
    }
    
    auto stmt = db_->prepare(R"(
        SELECT id, name, category, width, height, depth, base_price, 
               model_path, thumbnail_path, specifications, created_at, updated_at
        FROM catalog_items 
        ORDER BY category, name
    )");
    
    if (!stmt || !stmt->isValid()) {
        LOG_ERROR("Failed to prepare get all items statement");
        return items;
    }
    
    while (stmt->step()) {
        Models::CatalogItem item = resultToCatalogItem(*stmt);
        loadMaterialOptionsForItem(item);
        items.push_back(std::move(item));
    }
    
    return items;
}

std::vector<Models::CatalogItem> SQLiteCatalogRepository::getItemsByCategory(const std::string& category) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<Models::CatalogItem> items;
    
    if (!db_ || !db_->isOpen()) {
        LOG_ERROR("Database is not open");
        return items;
    }
    
    auto stmt = db_->prepare(R"(
        SELECT id, name, category, width, height, depth, base_price, 
               model_path, thumbnail_path, specifications, created_at, updated_at
        FROM catalog_items 
        WHERE category = ?
        ORDER BY name
    )");
    
    if (!stmt || !stmt->isValid()) {
        LOG_ERROR("Failed to prepare get items by category statement");
        return items;
    }
    
    stmt->bindText(1, category);
    
    while (stmt->step()) {
        Models::CatalogItem item = resultToCatalogItem(*stmt);
        loadMaterialOptionsForItem(item);
        items.push_back(std::move(item));
    }
    
    return items;
}

Models::CatalogSearchResult SQLiteCatalogRepository::searchItems(const Models::CatalogFilter& filter, size_t offset, size_t limit) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    Models::CatalogSearchResult result;
    
    if (!db_ || !db_->isOpen()) {
        LOG_ERROR("Database is not open");
        return result;
    }
    
    // Get total count
    std::string countQuery = buildSearchQuery(filter, true);
    auto countStmt = db_->prepare(countQuery);
    if (countStmt && countStmt->isValid()) {
        bindSearchParameters(*countStmt, filter);
        if (countStmt->step()) {
            result.totalCount = static_cast<size_t>(countStmt->getColumnInt64(0));
        }
    }
    
    // Get items
    std::string searchQuery = buildSearchQuery(filter, false);
    searchQuery += " ORDER BY category, name LIMIT ? OFFSET ?";
    
    auto stmt = db_->prepare(searchQuery);
    if (!stmt || !stmt->isValid()) {
        LOG_ERROR("Failed to prepare search items statement");
        return result;
    }
    
    bindSearchParameters(*stmt, filter);
    stmt->bindInt64(stmt->getColumnCount() - 1, static_cast<int64_t>(limit));
    stmt->bindInt64(stmt->getColumnCount(), static_cast<int64_t>(offset));
    
    while (stmt->step()) {
        Models::CatalogItem item = resultToCatalogItem(*stmt);
        loadMaterialOptionsForItem(item);
        
        // Apply additional filtering that can't be done in SQL
        if (filter.matches(item)) {
            result.items.push_back(std::make_shared<Models::CatalogItem>(std::move(item)));
        }
    }
    
    result.offset = offset;
    result.limit = limit;
    
    return result;
}

std::vector<std::string> SQLiteCatalogRepository::getCategories() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> categories;
    
    if (!db_ || !db_->isOpen()) {
        LOG_ERROR("Database is not open");
        return categories;
    }
    
    auto stmt = db_->prepare("SELECT DISTINCT category FROM catalog_items ORDER BY category");
    if (!stmt || !stmt->isValid()) {
        LOG_ERROR("Failed to prepare get categories statement");
        return categories;
    }
    
    while (stmt->step()) {
        categories.push_back(stmt->getColumnText(0));
    }
    
    return categories;
}

bool SQLiteCatalogRepository::addMaterialOption(const std::string& itemId, const Models::MaterialOption& option) {
    auto stmt = db_->prepare(R"(
        INSERT INTO material_options (id, catalog_item_id, name, texture_path, price_modifier, properties)
        VALUES (?, ?, ?, ?, ?, ?)
    )");
    
    if (!stmt || !stmt->isValid()) {
        LOG_ERROR("Failed to prepare add material option statement");
        return false;
    }
    
    stmt->bindText(1, option.id);
    stmt->bindText(2, itemId);
    stmt->bindText(3, option.name);
    stmt->bindText(4, option.texturePath);
    stmt->bindDouble(5, option.priceModifier);
    stmt->bindText(6, option.properties);
    
    return stmt->step();
}

bool SQLiteCatalogRepository::updateMaterialOption(const Models::MaterialOption& option) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!db_ || !db_->isOpen()) {
        LOG_ERROR("Database is not open");
        return false;
    }
    
    auto stmt = db_->prepare(R"(
        UPDATE material_options 
        SET name = ?, texture_path = ?, price_modifier = ?, properties = ?
        WHERE id = ?
    )");
    
    if (!stmt || !stmt->isValid()) {
        LOG_ERROR("Failed to prepare update material option statement");
        return false;
    }
    
    stmt->bindText(1, option.name);
    stmt->bindText(2, option.texturePath);
    stmt->bindDouble(3, option.priceModifier);
    stmt->bindText(4, option.properties);
    stmt->bindText(5, option.id);
    
    if (!stmt->step()) {
        LOG_ERROR("Failed to update material option: " + option.id);
        return false;
    }
    
    return db_->getChanges() > 0;
}

bool SQLiteCatalogRepository::deleteMaterialOption(const std::string& optionId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!db_ || !db_->isOpen()) {
        LOG_ERROR("Database is not open");
        return false;
    }
    
    auto stmt = db_->prepare("DELETE FROM material_options WHERE id = ?");
    if (!stmt || !stmt->isValid()) {
        LOG_ERROR("Failed to prepare delete material option statement");
        return false;
    }
    
    stmt->bindText(1, optionId);
    if (!stmt->step()) {
        LOG_ERROR("Failed to delete material option: " + optionId);
        return false;
    }
    
    return db_->getChanges() > 0;
}

std::vector<Models::MaterialOption> SQLiteCatalogRepository::getMaterialOptions(const std::string& itemId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<Models::MaterialOption> options;
    
    if (!db_ || !db_->isOpen()) {
        LOG_ERROR("Database is not open");
        return options;
    }
    
    auto stmt = db_->prepare(R"(
        SELECT id, name, texture_path, price_modifier, properties
        FROM material_options 
        WHERE catalog_item_id = ?
        ORDER BY name
    )");
    
    if (!stmt || !stmt->isValid()) {
        LOG_ERROR("Failed to prepare get material options statement");
        return options;
    }
    
    stmt->bindText(1, itemId);
    
    while (stmt->step()) {
        options.push_back(resultToMaterialOption(*stmt));
    }
    
    return options;
}

bool SQLiteCatalogRepository::importCatalog(const std::vector<Models::CatalogItem>& items) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!db_ || !db_->isOpen()) {
        LOG_ERROR("Database is not open");
        return false;
    }
    
    Transaction transaction(*db_);
    
    for (const auto& item : items) {
        if (!insertCatalogItem(item)) {
            LOG_ERROR("Failed to import catalog item: " + item.getId());
            return false;
        }
        
        for (const auto& option : item.getMaterialOptions()) {
            if (!addMaterialOption(item.getId(), option)) {
                LOG_ERROR("Failed to import material option: " + option.id);
                return false;
            }
        }
    }
    
    if (!transaction.commit()) {
        LOG_ERROR("Failed to commit catalog import");
        return false;
    }
    
    LOG_INFO("Imported " + std::to_string(items.size()) + " catalog items");
    return true;
}

std::vector<Models::CatalogItem> SQLiteCatalogRepository::exportCatalog() {
    return getAllItems();
}

bool SQLiteCatalogRepository::clearCatalog() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!db_ || !db_->isOpen()) {
        LOG_ERROR("Database is not open");
        return false;
    }
    
    Transaction transaction(*db_);
    
    // Delete all catalog items (cascade will handle material options)
    if (!db_->execute("DELETE FROM catalog_items")) {
        LOG_ERROR("Failed to clear catalog");
        return false;
    }
    
    if (!transaction.commit()) {
        LOG_ERROR("Failed to commit catalog clear");
        return false;
    }
    
    LOG_INFO("Cleared catalog");
    return true;
}

size_t SQLiteCatalogRepository::getItemCount() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!db_ || !db_->isOpen()) {
        return 0;
    }
    
    auto stmt = db_->prepare("SELECT COUNT(*) FROM catalog_items");
    if (!stmt || !stmt->isValid()) {
        return 0;
    }
    
    if (stmt->step()) {
        return static_cast<size_t>(stmt->getColumnInt64(0));
    }
    
    return 0;
}

size_t SQLiteCatalogRepository::getItemCountByCategory(const std::string& category) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!db_ || !db_->isOpen()) {
        return 0;
    }
    
    auto stmt = db_->prepare("SELECT COUNT(*) FROM catalog_items WHERE category = ?");
    if (!stmt || !stmt->isValid()) {
        return 0;
    }
    
    stmt->bindText(1, category);
    
    if (stmt->step()) {
        return static_cast<size_t>(stmt->getColumnInt64(0));
    }
    
    return 0;
}

bool SQLiteCatalogRepository::itemExists(const std::string& itemId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!db_ || !db_->isOpen()) {
        return false;
    }
    
    auto stmt = db_->prepare("SELECT 1 FROM catalog_items WHERE id = ?");
    if (!stmt || !stmt->isValid()) {
        return false;
    }
    
    stmt->bindText(1, itemId);
    return stmt->step();
}

bool SQLiteCatalogRepository::isValidItemName(const std::string& name) {
    if (name.empty() || name.length() > 255) {
        return false;
    }
    
    // Check for invalid characters
    const std::string invalidChars = "<>:\"/\\|?*";
    return name.find_first_of(invalidChars) == std::string::npos;
}

// Private helper methods
bool SQLiteCatalogRepository::insertCatalogItem(const Models::CatalogItem& item) {
    auto stmt = db_->prepare(R"(
        INSERT INTO catalog_items (id, name, category, width, height, depth, base_price, 
                                  model_path, thumbnail_path, specifications)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");
    
    if (!stmt || !stmt->isValid()) {
        LOG_ERROR("Failed to prepare insert catalog item statement");
        return false;
    }
    
    json specifications = item.getSpecifications().toJson();
    
    stmt->bindText(1, item.getId());
    stmt->bindText(2, item.getName());
    stmt->bindText(3, item.getCategory());
    stmt->bindDouble(4, item.getDimensions().width);
    stmt->bindDouble(5, item.getDimensions().height);
    stmt->bindDouble(6, item.getDimensions().depth);
    stmt->bindDouble(7, item.getBasePrice());
    stmt->bindText(8, item.getModelPath());
    stmt->bindText(9, item.getThumbnailPath());
    stmt->bindText(10, specifications.dump());
    
    return stmt->step();
}

bool SQLiteCatalogRepository::updateCatalogItem(const Models::CatalogItem& item) {
    auto stmt = db_->prepare(R"(
        UPDATE catalog_items 
        SET name = ?, category = ?, width = ?, height = ?, depth = ?, base_price = ?, 
            model_path = ?, thumbnail_path = ?, specifications = ?, updated_at = datetime('now')
        WHERE id = ?
    )");
    
    if (!stmt || !stmt->isValid()) {
        LOG_ERROR("Failed to prepare update catalog item statement");
        return false;
    }
    
    json specifications = item.getSpecifications().toJson();
    
    stmt->bindText(1, item.getName());
    stmt->bindText(2, item.getCategory());
    stmt->bindDouble(3, item.getDimensions().width);
    stmt->bindDouble(4, item.getDimensions().height);
    stmt->bindDouble(5, item.getDimensions().depth);
    stmt->bindDouble(6, item.getBasePrice());
    stmt->bindText(7, item.getModelPath());
    stmt->bindText(8, item.getThumbnailPath());
    stmt->bindText(9, specifications.dump());
    stmt->bindText(10, item.getId());
    
    return stmt->step();
}

std::optional<Models::CatalogItem> SQLiteCatalogRepository::loadCatalogItemFromDatabase(const std::string& itemId) {
    auto stmt = db_->prepare(R"(
        SELECT id, name, category, width, height, depth, base_price, 
               model_path, thumbnail_path, specifications, created_at, updated_at
        FROM catalog_items 
        WHERE id = ?
    )");
    
    if (!stmt || !stmt->isValid()) {
        LOG_ERROR("Failed to prepare load catalog item statement");
        return std::nullopt;
    }
    
    stmt->bindText(1, itemId);
    
    if (!stmt->step()) {
        return std::nullopt;
    }
    
    return resultToCatalogItem(*stmt);
}

bool SQLiteCatalogRepository::loadMaterialOptionsForItem(Models::CatalogItem& item) {
    auto options = getMaterialOptions(item.getId());
    
    item.clearMaterialOptions();
    for (const auto& option : options) {
        item.addMaterialOption(option);
    }
    
    return true;
}

std::string SQLiteCatalogRepository::buildSearchQuery(const Models::CatalogFilter& filter, bool countOnly) const {
    std::string query;
    
    if (countOnly) {
        query = "SELECT COUNT(*) FROM catalog_items WHERE 1=1";
    } else {
        query = R"(
            SELECT id, name, category, width, height, depth, base_price, 
                   model_path, thumbnail_path, specifications, created_at, updated_at
            FROM catalog_items WHERE 1=1
        )";
    }
    
    // Add search term filter
    if (!filter.searchTerm.empty()) {
        query += " AND (name LIKE ? OR category LIKE ?)";
    }
    
    // Add category filter
    if (!filter.categories.empty()) {
        query += " AND category IN (";
        for (size_t i = 0; i < filter.categories.size(); ++i) {
            if (i > 0) query += ",";
            query += "?";
        }
        query += ")";
    }
    
    // Add dimension filters
    if (filter.minDimensions.width > 0 || filter.maxDimensions.width < std::numeric_limits<double>::max()) {
        query += " AND width >= ? AND width <= ?";
    }
    if (filter.minDimensions.height > 0 || filter.maxDimensions.height < std::numeric_limits<double>::max()) {
        query += " AND height >= ? AND height <= ?";
    }
    if (filter.minDimensions.depth > 0 || filter.maxDimensions.depth < std::numeric_limits<double>::max()) {
        query += " AND depth >= ? AND depth <= ?";
    }
    
    // Add price filter
    if (filter.minPrice > 0 || filter.maxPrice < std::numeric_limits<double>::max()) {
        query += " AND base_price >= ? AND base_price <= ?";
    }
    
    return query;
}

void SQLiteCatalogRepository::bindSearchParameters(DatabaseManager::PreparedStatement& stmt, const Models::CatalogFilter& filter) const {
    int paramIndex = 1;
    
    // Bind search term
    if (!filter.searchTerm.empty()) {
        std::string searchPattern = "%" + filter.searchTerm + "%";
        stmt.bindText(paramIndex++, searchPattern);
        stmt.bindText(paramIndex++, searchPattern);
    }
    
    // Bind categories
    for (const auto& category : filter.categories) {
        stmt.bindText(paramIndex++, category);
    }
    
    // Bind dimensions
    if (filter.minDimensions.width > 0 || filter.maxDimensions.width < std::numeric_limits<double>::max()) {
        stmt.bindDouble(paramIndex++, filter.minDimensions.width);
        stmt.bindDouble(paramIndex++, filter.maxDimensions.width);
    }
    if (filter.minDimensions.height > 0 || filter.maxDimensions.height < std::numeric_limits<double>::max()) {
        stmt.bindDouble(paramIndex++, filter.minDimensions.height);
        stmt.bindDouble(paramIndex++, filter.maxDimensions.height);
    }
    if (filter.minDimensions.depth > 0 || filter.maxDimensions.depth < std::numeric_limits<double>::max()) {
        stmt.bindDouble(paramIndex++, filter.minDimensions.depth);
        stmt.bindDouble(paramIndex++, filter.maxDimensions.depth);
    }
    
    // Bind price range
    if (filter.minPrice > 0 || filter.maxPrice < std::numeric_limits<double>::max()) {
        stmt.bindDouble(paramIndex++, filter.minPrice);
        stmt.bindDouble(paramIndex++, filter.maxPrice);
    }
}

Models::CatalogItem SQLiteCatalogRepository::resultToCatalogItem(DatabaseManager::PreparedStatement& stmt) const {
    Models::CatalogItem item(
        stmt.getColumnText(0),  // id
        stmt.getColumnText(1),  // name
        stmt.getColumnText(2)   // category
    );
    
    // Dimensions
    Models::Dimensions3D dimensions(
        stmt.getColumnDouble(3),  // width
        stmt.getColumnDouble(4),  // height
        stmt.getColumnDouble(5)   // depth
    );
    item.setDimensions(dimensions);
    
    item.setBasePrice(stmt.getColumnDouble(6));
    item.setModelPath(stmt.getColumnText(7));
    item.setThumbnailPath(stmt.getColumnText(8));
    
    // Specifications
    std::string specificationsStr = stmt.getColumnText(9);
    if (!specificationsStr.empty()) {
        try {
            json specificationsJson = json::parse(specificationsStr);
            Models::Specifications specs;
            specs.fromJson(specificationsJson);
            item.setSpecifications(specs);
        } catch (const std::exception& e) {
            LOG_WARNING("Failed to parse specifications for item " + item.getId() + ": " + e.what());
        }
    }
    
    return item;
}

Models::MaterialOption SQLiteCatalogRepository::resultToMaterialOption(DatabaseManager::PreparedStatement& stmt) const {
    Models::MaterialOption option;
    option.id = stmt.getColumnText(0);
    option.name = stmt.getColumnText(1);
    option.texturePath = stmt.getColumnText(2);
    option.priceModifier = stmt.getColumnDouble(3);
    option.properties = stmt.getColumnText(4);
    
    return option;
}

} // namespace Persistence
} // namespace KitchenCAD