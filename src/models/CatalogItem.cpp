#include "CatalogItem.h"
#include "../utils/Logger.h"
#include <nlohmann/json.hpp>
#include <random>
#include <sstream>
#include <algorithm>
#include <limits>

namespace KitchenCAD {
namespace Models {

using json = nlohmann::json;

// MaterialOption implementation
json MaterialOption::toJson() const {
    json j;
    j["id"] = id;
    j["name"] = name;
    j["texturePath"] = texturePath;
    j["priceModifier"] = priceModifier;
    j["properties"] = properties;
    return j;
}

void MaterialOption::fromJson(const json& j) {
    if (j.contains("id")) id = j["id"];
    if (j.contains("name")) name = j["name"];
    if (j.contains("texturePath")) texturePath = j["texturePath"];
    if (j.contains("priceModifier")) priceModifier = j["priceModifier"];
    if (j.contains("properties")) properties = j["properties"];
}

// Specifications implementation
json Specifications::toJson() const {
    json j;
    j["material"] = material;
    j["finish"] = finish;
    j["hardware"] = hardware;
    j["weight"] = weight;
    j["loadCapacity"] = loadCapacity;
    j["installationType"] = installationType;
    j["features"] = features;
    j["additionalInfo"] = additionalInfo;
    return j;
}

void Specifications::fromJson(const json& j) {
    if (j.contains("material")) material = j["material"];
    if (j.contains("finish")) finish = j["finish"];
    if (j.contains("hardware")) hardware = j["hardware"];
    if (j.contains("weight")) weight = j["weight"];
    if (j.contains("loadCapacity")) loadCapacity = j["loadCapacity"];
    if (j.contains("installationType")) installationType = j["installationType"];
    if (j.contains("features")) features = j["features"];
    if (j.contains("additionalInfo")) additionalInfo = j["additionalInfo"];
}

// CatalogItem implementation
CatalogItem::CatalogItem(const std::string& id, const std::string& name, const std::string& category)
    : id_(id), name_(name), category_(category), basePrice_(0.0) {
    initializeTimestamps();
}

void CatalogItem::addMaterialOption(const MaterialOption& option) {
    // Remove existing option with same ID
    removeMaterialOption(option.id);
    
    materialOptions_.push_back(option);
    updateTimestamp();
}

bool CatalogItem::removeMaterialOption(const std::string& optionId) {
    auto it = std::find_if(materialOptions_.begin(), materialOptions_.end(),
        [&optionId](const MaterialOption& option) {
            return option.id == optionId;
        });
    
    if (it != materialOptions_.end()) {
        materialOptions_.erase(it);
        updateTimestamp();
        return true;
    }
    
    return false;
}

const MaterialOption* CatalogItem::getMaterialOption(const std::string& optionId) const {
    auto it = std::find_if(materialOptions_.begin(), materialOptions_.end(),
        [&optionId](const MaterialOption& option) {
            return option.id == optionId;
        });
    
    return (it != materialOptions_.end()) ? &(*it) : nullptr;
}

double CatalogItem::getPrice(const std::string& materialId) const {
    double price = basePrice_;
    
    if (!materialId.empty()) {
        const MaterialOption* option = getMaterialOption(materialId);
        if (option) {
            price += option->priceModifier;
        }
    }
    
    return std::max(0.0, price);
}

double CatalogItem::getPriceWithMaterial(const MaterialOption& material) const {
    return std::max(0.0, basePrice_ + material.priceModifier);
}

bool CatalogItem::isValid() const {
    return !id_.empty() && 
           !name_.empty() && 
           !category_.empty() && 
           dimensions_.isValid() && 
           basePrice_ >= 0.0;
}

std::vector<std::string> CatalogItem::validate() const {
    std::vector<std::string> errors;
    
    if (id_.empty()) {
        errors.push_back("Item ID cannot be empty");
    }
    
    if (name_.empty()) {
        errors.push_back("Item name cannot be empty");
    }
    
    if (category_.empty()) {
        errors.push_back("Item category cannot be empty");
    }
    
    if (!dimensions_.isValid()) {
        errors.push_back("Invalid dimensions");
    }
    
    if (basePrice_ < 0.0) {
        errors.push_back("Base price cannot be negative");
    }
    
    // Validate material options
    for (const auto& option : materialOptions_) {
        if (option.id.empty()) {
            errors.push_back("Material option ID cannot be empty");
        }
        if (option.name.empty()) {
            errors.push_back("Material option name cannot be empty");
        }
    }
    
    return errors;
}

bool CatalogItem::matchesSearch(const std::string& searchTerm) const {
    if (searchTerm.empty()) {
        return true;
    }
    
    std::string lowerSearchTerm = searchTerm;
    std::transform(lowerSearchTerm.begin(), lowerSearchTerm.end(), lowerSearchTerm.begin(), ::tolower);
    
    // Check name
    std::string lowerName = name_;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    if (lowerName.find(lowerSearchTerm) != std::string::npos) {
        return true;
    }
    
    // Check category
    std::string lowerCategory = category_;
    std::transform(lowerCategory.begin(), lowerCategory.end(), lowerCategory.begin(), ::tolower);
    if (lowerCategory.find(lowerSearchTerm) != std::string::npos) {
        return true;
    }
    
    // Check specifications
    std::string lowerMaterial = specifications_.material;
    std::transform(lowerMaterial.begin(), lowerMaterial.end(), lowerMaterial.begin(), ::tolower);
    if (lowerMaterial.find(lowerSearchTerm) != std::string::npos) {
        return true;
    }
    
    // Check features
    for (const auto& feature : specifications_.features) {
        std::string lowerFeature = feature;
        std::transform(lowerFeature.begin(), lowerFeature.end(), lowerFeature.begin(), ::tolower);
        if (lowerFeature.find(lowerSearchTerm) != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

bool CatalogItem::matchesCategory(const std::string& category) const {
    return category.empty() || category_ == category;
}

bool CatalogItem::matchesDimensions(const Dimensions3D& minDims, const Dimensions3D& maxDims) const {
    return dimensions_.width >= minDims.width && dimensions_.width <= maxDims.width &&
           dimensions_.height >= minDims.height && dimensions_.height <= maxDims.height &&
           dimensions_.depth >= minDims.depth && dimensions_.depth <= maxDims.depth;
}

bool CatalogItem::matchesPriceRange(double minPrice, double maxPrice) const {
    double price = getPrice();
    return price >= minPrice && price <= maxPrice;
}

json CatalogItem::toJson() const {
    json j;
    
    j["id"] = id_;
    j["name"] = name_;
    j["category"] = category_;
    j["basePrice"] = basePrice_;
    j["modelPath"] = modelPath_;
    j["thumbnailPath"] = thumbnailPath_;
    
    // Dimensions
    j["dimensions"] = {
        {"width", dimensions_.width},
        {"height", dimensions_.height},
        {"depth", dimensions_.depth}
    };
    
    // Specifications
    j["specifications"] = specifications_.toJson();
    
    // Material options
    j["materialOptions"] = json::array();
    for (const auto& option : materialOptions_) {
        j["materialOptions"].push_back(option.toJson());
    }
    
    // Timestamps
    auto createdTime = std::chrono::system_clock::to_time_t(createdAt_);
    auto updatedTime = std::chrono::system_clock::to_time_t(updatedAt_);
    
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&createdTime), "%Y-%m-%d %H:%M:%S");
    j["createdAt"] = ss.str();
    
    ss.str("");
    ss << std::put_time(std::gmtime(&updatedTime), "%Y-%m-%d %H:%M:%S");
    j["updatedAt"] = ss.str();
    
    return j;
}

void CatalogItem::fromJson(const json& j) {
    if (j.contains("id")) id_ = j["id"];
    if (j.contains("name")) name_ = j["name"];
    if (j.contains("category")) category_ = j["category"];
    if (j.contains("basePrice")) basePrice_ = j["basePrice"];
    if (j.contains("modelPath")) modelPath_ = j["modelPath"];
    if (j.contains("thumbnailPath")) thumbnailPath_ = j["thumbnailPath"];
    
    // Dimensions
    if (j.contains("dimensions")) {
        const auto& dims = j["dimensions"];
        dimensions_ = Dimensions3D(dims["width"], dims["height"], dims["depth"]);
    }
    
    // Specifications
    if (j.contains("specifications")) {
        specifications_.fromJson(j["specifications"]);
    }
    
    // Material options
    materialOptions_.clear();
    if (j.contains("materialOptions")) {
        for (const auto& optionJson : j["materialOptions"]) {
            MaterialOption option;
            option.fromJson(optionJson);
            materialOptions_.push_back(option);
        }
    }
    
    // Timestamps (simplified parsing)
    if (j.contains("createdAt")) {
        // In a full implementation, would parse the timestamp properly
        createdAt_ = std::chrono::system_clock::now();
    }
    if (j.contains("updatedAt")) {
        updatedAt_ = std::chrono::system_clock::now();
    }
}

std::string CatalogItem::generateId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    ss << "cat_";
    for (int i = 0; i < 8; ++i) {
        ss << std::hex << dis(gen);
    }
    return ss.str();
}

std::vector<std::string> CatalogItem::getStandardCategories() {
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
        "flooring",
        "backsplash"
    };
}

void CatalogItem::initializeTimestamps() {
    auto now = std::chrono::system_clock::now();
    createdAt_ = now;
    updatedAt_ = now;
}

// CatalogFilter implementation
bool CatalogFilter::matches(const CatalogItem& item) const {
    // Search term
    if (!searchTerm.empty() && !item.matchesSearch(searchTerm)) {
        return false;
    }
    
    // Categories
    if (!categories.empty()) {
        bool categoryMatch = false;
        for (const auto& category : categories) {
            if (item.matchesCategory(category)) {
                categoryMatch = true;
                break;
            }
        }
        if (!categoryMatch) {
            return false;
        }
    }
    
    // Dimensions
    if (!item.matchesDimensions(minDimensions, maxDimensions)) {
        return false;
    }
    
    // Price range
    if (!item.matchesPriceRange(minPrice, maxPrice)) {
        return false;
    }
    
    // Features
    if (!features.empty()) {
        const auto& itemFeatures = item.getSpecifications().features;
        for (const auto& requiredFeature : features) {
            if (std::find(itemFeatures.begin(), itemFeatures.end(), requiredFeature) == itemFeatures.end()) {
                return false;
            }
        }
    }
    
    return true;
}

json CatalogFilter::toJson() const {
    json j;
    j["searchTerm"] = searchTerm;
    j["categories"] = categories;
    j["minDimensions"] = {
        {"width", minDimensions.width},
        {"height", minDimensions.height},
        {"depth", minDimensions.depth}
    };
    j["maxDimensions"] = {
        {"width", maxDimensions.width},
        {"height", maxDimensions.height},
        {"depth", maxDimensions.depth}
    };
    j["minPrice"] = minPrice;
    j["maxPrice"] = maxPrice;
    j["features"] = features;
    return j;
}

void CatalogFilter::fromJson(const json& j) {
    if (j.contains("searchTerm")) searchTerm = j["searchTerm"];
    if (j.contains("categories")) categories = j["categories"];
    
    if (j.contains("minDimensions")) {
        const auto& dims = j["minDimensions"];
        minDimensions = Dimensions3D(dims["width"], dims["height"], dims["depth"]);
    }
    
    if (j.contains("maxDimensions")) {
        const auto& dims = j["maxDimensions"];
        maxDimensions = Dimensions3D(dims["width"], dims["height"], dims["depth"]);
    }
    
    if (j.contains("minPrice")) minPrice = j["minPrice"];
    if (j.contains("maxPrice")) maxPrice = j["maxPrice"];
    if (j.contains("features")) features = j["features"];
}

} // namespace Models
} // namespace KitchenCAD