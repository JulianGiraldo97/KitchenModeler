#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include "../geometry/Point3D.h"
#include "../geometry/Vector3D.h"
#include "../geometry/BoundingBox.h"
#include "../geometry/Transform3D.h"
#include "../interfaces/IProjectRepository.h"

#include <nlohmann/json.hpp>

namespace KitchenCAD {
namespace Models {

using namespace Geometry;

/**
 * @brief Room dimensions structure
 */
struct RoomDimensions {
    double width;
    double height; 
    double depth;
    
    RoomDimensions() : width(0.0), height(0.0), depth(0.0) {}
    RoomDimensions(double w, double h, double d) : width(w), height(h), depth(d) {}
    
    bool isValid() const {
        return width > 0.0 && height > 0.0 && depth > 0.0;
    }
    
    double volume() const {
        return width * height * depth;
    }
    
    BoundingBox toBoundingBox() const {
        return BoundingBox(Point3D(0, 0, 0), Point3D(width, height, depth));
    }
};

/**
 * @brief Wall structure for room layout
 */
struct Wall {
    std::string id;
    Point3D start;
    Point3D end;
    double height;
    double thickness;
    std::string materialId;
    
    Wall() : height(0.0), thickness(0.0) {}
    Wall(const std::string& id, const Point3D& start, const Point3D& end, double height, double thickness = 0.1)
        : id(id), start(start), end(end), height(height), thickness(thickness) {}
    
    double length() const {
        return start.distanceTo(end);
    }
    
    Vector3D direction() const {
        return Vector3D(start, end).normalized();
    }
    
    bool isValid() const {
        return length() > 0.01 && height > 0.0 && thickness > 0.0;
    }
};

/**
 * @brief Opening (door/window) in a wall
 */
struct Opening {
    std::string id;
    std::string wallId;
    std::string type; // "door" or "window"
    double position; // Position along wall (0.0 to 1.0)
    double width;
    double height;
    double sillHeight; // Height from floor (for windows)
    
    Opening() : position(0.0), width(0.0), height(0.0), sillHeight(0.0) {}
    Opening(const std::string& id, const std::string& wallId, const std::string& type,
            double pos, double w, double h, double sill = 0.0)
        : id(id), wallId(wallId), type(type), position(pos), width(w), height(h), sillHeight(sill) {}
    
    bool isValid() const {
        return position >= 0.0 && position <= 1.0 && width > 0.0 && height > 0.0 && sillHeight >= 0.0;
    }
};

// Use the geometry Transform3D instead of defining our own
using Transform3D = Geometry::Transform3D;

/**
 * @brief Material properties for objects
 */
struct MaterialProperties {
    std::string id;
    std::string name;
    std::string texturePath;
    
    // Visual properties
    struct Color {
        float r, g, b, a;
        Color() : r(1.0f), g(1.0f), b(1.0f), a(1.0f) {}
        Color(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}
    } diffuseColor, specularColor;
    
    float roughness = 0.5f;
    float metallic = 0.0f;
    float reflectance = 0.04f;
    
    // Economic properties
    double pricePerSquareMeter = 0.0;
    std::string supplier;
    std::string code;
    
    MaterialProperties() = default;
    MaterialProperties(const std::string& id, const std::string& name)
        : id(id), name(name) {}
};

/**
 * @brief Base class for objects in the scene
 */
class SceneObject {
protected:
    std::string id_;
    std::string catalogItemId_;
    Transform3D transform_;
    MaterialProperties material_;
    std::string customProperties_; // JSON string for additional properties
    
public:
    SceneObject() = default;
    SceneObject(const std::string& catalogItemId);
    virtual ~SceneObject() = default;
    
    // Basic properties
    const std::string& getId() const { return id_; }
    void setId(const std::string& id) { id_ = id; }
    
    const std::string& getCatalogItemId() const { return catalogItemId_; }
    void setCatalogItemId(const std::string& id) { catalogItemId_ = id; }
    
    const Transform3D& getTransform() const { return transform_; }
    void setTransform(const Transform3D& transform) { transform_ = transform; }
    
    const MaterialProperties& getMaterial() const { return material_; }
    void setMaterial(const MaterialProperties& material) { material_ = material; }
    
    const std::string& getCustomProperties() const { return customProperties_; }
    void setCustomProperties(const std::string& properties) { customProperties_ = properties; }
    
    // Serialization
    virtual nlohmann::json toJson() const;
    virtual void fromJson(const nlohmann::json& json);
    
    // Utility
    static std::string generateId();
};

/**
 * @brief Main project class containing all design data
 */
class Project {
private:
    std::string id_;
    std::string name_;
    std::string description_;
    RoomDimensions dimensions_;
    std::vector<std::unique_ptr<SceneObject>> objects_;
    std::vector<Wall> walls_;
    std::vector<Opening> openings_;
    std::string thumbnailPath_;
    
    // Timestamps
    std::chrono::system_clock::time_point createdAt_;
    std::chrono::system_clock::time_point updatedAt_;
    
public:
    Project() = default;
    Project(const std::string& name, const RoomDimensions& dimensions);
    Project(const std::string& id, const std::string& name, const RoomDimensions& dimensions);
    
    // Non-copyable but movable
    Project(const Project&) = delete;
    Project& operator=(const Project&) = delete;
    Project(Project&&) = default;
    Project& operator=(Project&&) = default;
    
    // Basic properties
    const std::string& getId() const { return id_; }
    void setId(const std::string& id) { id_ = id; }
    
    const std::string& getName() const { return name_; }
    void setName(const std::string& name) { name_ = name; updateTimestamp(); }
    
    const std::string& getDescription() const { return description_; }
    void setDescription(const std::string& description) { description_ = description; updateTimestamp(); }
    
    const RoomDimensions& getDimensions() const { return dimensions_; }
    void setDimensions(const RoomDimensions& dimensions) { dimensions_ = dimensions; updateTimestamp(); }
    
    const std::string& getThumbnailPath() const { return thumbnailPath_; }
    void setThumbnailPath(const std::string& path) { thumbnailPath_ = path; }
    
    // Timestamps
    std::chrono::system_clock::time_point getCreatedAt() const { return createdAt_; }
    std::chrono::system_clock::time_point getUpdatedAt() const { return updatedAt_; }
    void updateTimestamp() { updatedAt_ = std::chrono::system_clock::now(); }
    
    // Object management
    const std::vector<std::unique_ptr<SceneObject>>& getObjects() const { return objects_; }
    std::string addObject(std::unique_ptr<SceneObject> object);
    bool removeObject(const std::string& objectId);
    SceneObject* getObject(const std::string& objectId);
    const SceneObject* getObject(const std::string& objectId) const;
    size_t getObjectCount() const { return objects_.size(); }
    
    // Wall management
    const std::vector<Wall>& getWalls() const { return walls_; }
    void addWall(const Wall& wall);
    bool removeWall(const std::string& wallId);
    Wall* getWall(const std::string& wallId);
    const Wall* getWall(const std::string& wallId) const;
    
    // Opening management
    const std::vector<Opening>& getOpenings() const { return openings_; }
    void addOpening(const Opening& opening);
    bool removeOpening(const std::string& openingId);
    Opening* getOpening(const std::string& openingId);
    const Opening* getOpening(const std::string& openingId) const;
    
    // Calculations
    double calculateTotalPrice() const;
    BoundingBox calculateBoundingBox() const;
    
    // Validation
    std::vector<std::string> validate() const;
    bool isValid() const { return validate().empty(); }
    
    // Serialization
    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& json);
    nlohmann::json serializeSceneToJson() const;
    void loadSceneFromJson(const nlohmann::json& json);
    
    // Conversion to ProjectInfo and ProjectMetadata
    ProjectInfo toProjectInfo() const;
    ProjectMetadata toProjectMetadata() const;
    void updateFromMetadata(const ProjectMetadata& metadata);
    
    // Utility
    static std::string generateId();
    
private:
    void initializeTimestamps();
};

} // namespace Models
} // namespace KitchenCAD