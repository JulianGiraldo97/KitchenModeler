#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include "../geometry/Point3D.h"
#include "../geometry/Vector3D.h"
#include "../geometry/BoundingBox.h"

#include <nlohmann/json.hpp>

namespace KitchenCAD {
namespace Models {

using namespace Geometry;

/**
 * @brief 3D dimensions structure
 */
struct Dimensions3D {
    double width;
    double height;
    double depth;
    
    Dimensions3D() : width(0.0), height(0.0), depth(0.0) {}
    Dimensions3D(double w, double h, double d) : width(w), height(h), depth(d) {}
    
    bool isValid() const {
        return width > 0.0 && height > 0.0 && depth > 0.0;
    }
    
    double volume() const {
        return width * height * depth;
    }
    
    Vector3D toVector() const {
        return Vector3D(width, height, depth);
    }
    
    BoundingBox toBoundingBox(const Point3D& origin = Point3D(0, 0, 0)) const {
        return BoundingBox(origin, Point3D(origin.x + width, origin.y + height, origin.z + depth));
    }
};

/**
 * @brief Material option for catalog items
 */
struct MaterialOption {
    std::string id;
    std::string name;
    std::string texturePath;
    double priceModifier; // Additional cost or discount
    std::string properties; // JSON string for additional properties
    
    MaterialOption() : priceModifier(0.0) {}
    MaterialOption(const std::string& id, const std::string& name, double priceModifier = 0.0)
        : id(id), name(name), priceModifier(priceModifier) {}
    
    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& json);
};

/**
 * @brief Technical specifications for catalog items
 */
struct Specifications {
    std::string material;
    std::string finish;
    std::string hardware;
    double weight;
    double loadCapacity;
    std::string installationType;
    std::vector<std::string> features;
    std::string additionalInfo; // JSON string for custom specifications
    
    Specifications() : weight(0.0), loadCapacity(0.0) {}
    
    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& json);
};

/**
 * @brief Catalog item representing a modular component
 */
class CatalogItem {
private:
    std::string id_;
    std::string name_;
    std::string category_;
    Dimensions3D dimensions_;
    double basePrice_;
    std::vector<MaterialOption> materialOptions_;
    Specifications specifications_;
    std::string modelPath_;
    std::string thumbnailPath_;
    
    // Timestamps
    std::chrono::system_clock::time_point createdAt_;
    std::chrono::system_clock::time_point updatedAt_;
    
public:
    CatalogItem() : basePrice_(0.0) { initializeTimestamps(); }
    CatalogItem(const std::string& id, const std::string& name, const std::string& category);
    
    // Basic properties
    const std::string& getId() const { return id_; }
    void setId(const std::string& id) { id_ = id; }
    
    const std::string& getName() const { return name_; }
    void setName(const std::string& name) { name_ = name; updateTimestamp(); }
    
    const std::string& getCategory() const { return category_; }
    void setCategory(const std::string& category) { category_ = category; updateTimestamp(); }
    
    const Dimensions3D& getDimensions() const { return dimensions_; }
    void setDimensions(const Dimensions3D& dimensions) { dimensions_ = dimensions; updateTimestamp(); }
    
    double getBasePrice() const { return basePrice_; }
    void setBasePrice(double price) { basePrice_ = price; updateTimestamp(); }
    
    const std::string& getModelPath() const { return modelPath_; }
    void setModelPath(const std::string& path) { modelPath_ = path; updateTimestamp(); }
    
    const std::string& getThumbnailPath() const { return thumbnailPath_; }
    void setThumbnailPath(const std::string& path) { thumbnailPath_ = path; updateTimestamp(); }
    
    const Specifications& getSpecifications() const { return specifications_; }
    void setSpecifications(const Specifications& specs) { specifications_ = specs; updateTimestamp(); }
    
    // Timestamps
    std::chrono::system_clock::time_point getCreatedAt() const { return createdAt_; }
    std::chrono::system_clock::time_point getUpdatedAt() const { return updatedAt_; }
    void updateTimestamp() { updatedAt_ = std::chrono::system_clock::now(); }
    
    // Material options
    const std::vector<MaterialOption>& getMaterialOptions() const { return materialOptions_; }
    void addMaterialOption(const MaterialOption& option);
    bool removeMaterialOption(const std::string& optionId);
    const MaterialOption* getMaterialOption(const std::string& optionId) const;
    void clearMaterialOptions() { materialOptions_.clear(); updateTimestamp(); }
    
    // Price calculations
    double getPrice(const std::string& materialId = "") const;
    double getPriceWithMaterial(const MaterialOption& material) const;
    
    // Validation
    bool isValid() const;
    std::vector<std::string> validate() const;
    
    // Search and filtering
    bool matchesSearch(const std::string& searchTerm) const;
    bool matchesCategory(const std::string& category) const;
    bool matchesDimensions(const Dimensions3D& minDims, const Dimensions3D& maxDims) const;
    bool matchesPriceRange(double minPrice, double maxPrice) const;
    
    // Serialization
    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& json);
    
    // Utility
    static std::string generateId();
    static std::vector<std::string> getStandardCategories();
    
private:
    void initializeTimestamps();
};

/**
 * @brief Catalog filter criteria
 */
struct CatalogFilter {
    std::string searchTerm;
    std::vector<std::string> categories;
    Dimensions3D minDimensions;
    Dimensions3D maxDimensions;
    double minPrice = 0.0;
    double maxPrice = std::numeric_limits<double>::max();
    std::vector<std::string> features;
    
    CatalogFilter() = default;
    
    bool matches(const CatalogItem& item) const;
    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& json);
};

/**
 * @brief Catalog search result
 */
struct CatalogSearchResult {
    std::vector<std::shared_ptr<CatalogItem>> items;
    size_t totalCount;
    size_t offset;
    size_t limit;
    
    CatalogSearchResult() : totalCount(0), offset(0), limit(0) {}
    CatalogSearchResult(const std::vector<std::shared_ptr<CatalogItem>>& items, 
                       size_t total, size_t offset = 0, size_t limit = 0)
        : items(items), totalCount(total), offset(offset), limit(limit) {}
    
    bool hasMore() const { return offset + items.size() < totalCount; }
    size_t getReturnedCount() const { return items.size(); }
};

} // namespace Models
} // namespace KitchenCAD