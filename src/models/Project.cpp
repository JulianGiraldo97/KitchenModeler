#include "Project.h"
#include "../utils/Logger.h"
#include <nlohmann/json.hpp>
#include <random>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace KitchenCAD {
namespace Models {

using json = nlohmann::json;

// Transform3D is now using the geometry implementation

// SceneObject implementation
SceneObject::SceneObject(const std::string& catalogItemId) 
    : catalogItemId_(catalogItemId) {
    id_ = generateId();
}

json SceneObject::toJson() const {
    json j;
    j["id"] = id_;
    j["catalogItemId"] = catalogItemId_;
    
    // Transform
    j["transform"]["translation"] = {
        {"x", transform_.translation.x},
        {"y", transform_.translation.y},
        {"z", transform_.translation.z}
    };
    j["transform"]["rotation"] = {
        {"x", transform_.rotation.x},
        {"y", transform_.rotation.y},
        {"z", transform_.rotation.z}
    };
    j["transform"]["scale"] = {
        {"x", transform_.scale.x},
        {"y", transform_.scale.y},
        {"z", transform_.scale.z}
    };
    
    // Material
    j["material"]["id"] = material_.id;
    j["material"]["name"] = material_.name;
    j["material"]["texturePath"] = material_.texturePath;
    j["material"]["diffuseColor"] = {
        {"r", material_.diffuseColor.r},
        {"g", material_.diffuseColor.g},
        {"b", material_.diffuseColor.b},
        {"a", material_.diffuseColor.a}
    };
    j["material"]["roughness"] = material_.roughness;
    j["material"]["metallic"] = material_.metallic;
    j["material"]["pricePerSquareMeter"] = material_.pricePerSquareMeter;
    
    j["customProperties"] = customProperties_;
    
    return j;
}

void SceneObject::fromJson(const json& j) {
    if (j.contains("id")) {
        id_ = j["id"];
    }
    if (j.contains("catalogItemId")) {
        catalogItemId_ = j["catalogItemId"];
    }
    
    // Transform
    if (j.contains("transform")) {
        const auto& t = j["transform"];
        if (t.contains("translation")) {
            const auto& trans = t["translation"];
            transform_.translation = Point3D(trans["x"], trans["y"], trans["z"]);
        }
        if (t.contains("rotation")) {
            const auto& rot = t["rotation"];
            transform_.rotation = Vector3D(rot["x"], rot["y"], rot["z"]);
        }
        if (t.contains("scale")) {
            const auto& scl = t["scale"];
            transform_.scale = Vector3D(scl["x"], scl["y"], scl["z"]);
        }
    }
    
    // Material
    if (j.contains("material")) {
        const auto& m = j["material"];
        if (m.contains("id")) material_.id = m["id"];
        if (m.contains("name")) material_.name = m["name"];
        if (m.contains("texturePath")) material_.texturePath = m["texturePath"];
        if (m.contains("diffuseColor")) {
            const auto& color = m["diffuseColor"];
            material_.diffuseColor = {color["r"], color["g"], color["b"], color["a"]};
        }
        if (m.contains("roughness")) material_.roughness = m["roughness"];
        if (m.contains("metallic")) material_.metallic = m["metallic"];
        if (m.contains("pricePerSquareMeter")) material_.pricePerSquareMeter = m["pricePerSquareMeter"];
    }
    
    if (j.contains("customProperties")) {
        customProperties_ = j["customProperties"];
    }
}

std::string SceneObject::generateId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    ss << "obj_";
    for (int i = 0; i < 8; ++i) {
        ss << std::hex << dis(gen);
    }
    return ss.str();
}

// Project implementation
Project::Project(const std::string& name, const RoomDimensions& dimensions)
    : name_(name), dimensions_(dimensions) {
    id_ = generateId();
    initializeTimestamps();
}

Project::Project(const std::string& id, const std::string& name, const RoomDimensions& dimensions)
    : id_(id), name_(name), dimensions_(dimensions) {
    initializeTimestamps();
}

std::string Project::addObject(std::unique_ptr<SceneObject> object) {
    if (!object) {
        return "";
    }
    
    std::string objectId = object->getId();
    if (objectId.empty()) {
        objectId = SceneObject::generateId();
        object->setId(objectId);
    }
    
    objects_.push_back(std::move(object));
    updateTimestamp();
    
    return objectId;
}

bool Project::removeObject(const std::string& objectId) {
    auto it = std::find_if(objects_.begin(), objects_.end(),
        [&objectId](const std::unique_ptr<SceneObject>& obj) {
            return obj->getId() == objectId;
        });
    
    if (it != objects_.end()) {
        objects_.erase(it);
        updateTimestamp();
        return true;
    }
    
    return false;
}

SceneObject* Project::getObject(const std::string& objectId) {
    auto it = std::find_if(objects_.begin(), objects_.end(),
        [&objectId](const std::unique_ptr<SceneObject>& obj) {
            return obj->getId() == objectId;
        });
    
    return (it != objects_.end()) ? it->get() : nullptr;
}

const SceneObject* Project::getObject(const std::string& objectId) const {
    auto it = std::find_if(objects_.begin(), objects_.end(),
        [&objectId](const std::unique_ptr<SceneObject>& obj) {
            return obj->getId() == objectId;
        });
    
    return (it != objects_.end()) ? it->get() : nullptr;
}

void Project::addWall(const Wall& wall) {
    walls_.push_back(wall);
    updateTimestamp();
}

bool Project::removeWall(const std::string& wallId) {
    auto it = std::find_if(walls_.begin(), walls_.end(),
        [&wallId](const Wall& wall) {
            return wall.id == wallId;
        });
    
    if (it != walls_.end()) {
        walls_.erase(it);
        updateTimestamp();
        return true;
    }
    
    return false;
}

Wall* Project::getWall(const std::string& wallId) {
    auto it = std::find_if(walls_.begin(), walls_.end(),
        [&wallId](const Wall& wall) {
            return wall.id == wallId;
        });
    
    return (it != walls_.end()) ? &(*it) : nullptr;
}

const Wall* Project::getWall(const std::string& wallId) const {
    auto it = std::find_if(walls_.begin(), walls_.end(),
        [&wallId](const Wall& wall) {
            return wall.id == wallId;
        });
    
    return (it != walls_.end()) ? &(*it) : nullptr;
}

void Project::addOpening(const Opening& opening) {
    openings_.push_back(opening);
    updateTimestamp();
}

bool Project::removeOpening(const std::string& openingId) {
    auto it = std::find_if(openings_.begin(), openings_.end(),
        [&openingId](const Opening& opening) {
            return opening.id == openingId;
        });
    
    if (it != openings_.end()) {
        openings_.erase(it);
        updateTimestamp();
        return true;
    }
    
    return false;
}

Opening* Project::getOpening(const std::string& openingId) {
    auto it = std::find_if(openings_.begin(), openings_.end(),
        [&openingId](const Opening& opening) {
            return opening.id == openingId;
        });
    
    return (it != openings_.end()) ? &(*it) : nullptr;
}

const Opening* Project::getOpening(const std::string& openingId) const {
    auto it = std::find_if(openings_.begin(), openings_.end(),
        [&openingId](const Opening& opening) {
            return opening.id == openingId;
        });
    
    return (it != openings_.end()) ? &(*it) : nullptr;
}

double Project::calculateTotalPrice() const {
    double total = 0.0;
    
    for (const auto& object : objects_) {
        // This would need to be implemented with actual catalog lookup
        // For now, we'll use the material price as a placeholder
        total += object->getMaterial().pricePerSquareMeter;
    }
    
    return total;
}

BoundingBox Project::calculateBoundingBox() const {
    if (objects_.empty()) {
        return dimensions_.toBoundingBox();
    }
    
    // This would need proper geometry calculation
    // For now, return room dimensions
    return dimensions_.toBoundingBox();
}

std::vector<std::string> Project::validate() const {
    std::vector<std::string> errors;
    
    if (name_.empty()) {
        errors.push_back("Project name cannot be empty");
    }
    
    if (!dimensions_.isValid()) {
        errors.push_back("Invalid room dimensions");
    }
    
    // Validate walls
    for (const auto& wall : walls_) {
        if (!wall.isValid()) {
            errors.push_back("Invalid wall: " + wall.id);
        }
    }
    
    // Validate openings
    for (const auto& opening : openings_) {
        if (!opening.isValid()) {
            errors.push_back("Invalid opening: " + opening.id);
        }
        
        // Check if wall exists for opening
        if (getWall(opening.wallId) == nullptr) {
            errors.push_back("Opening references non-existent wall: " + opening.wallId);
        }
    }
    
    return errors;
}

json Project::toJson() const {
    json j;
    
    j["id"] = id_;
    j["name"] = name_;
    j["description"] = description_;
    j["thumbnailPath"] = thumbnailPath_;
    
    // Dimensions
    j["dimensions"] = {
        {"width", dimensions_.width},
        {"height", dimensions_.height},
        {"depth", dimensions_.depth}
    };
    
    // Timestamps
    auto createdTime = std::chrono::system_clock::to_time_t(createdAt_);
    auto updatedTime = std::chrono::system_clock::to_time_t(updatedAt_);
    
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&createdTime), "%Y-%m-%d %H:%M:%S");
    j["createdAt"] = ss.str();
    
    ss.str("");
    ss << std::put_time(std::gmtime(&updatedTime), "%Y-%m-%d %H:%M:%S");
    j["updatedAt"] = ss.str();
    
    // Objects
    j["objects"] = json::array();
    for (const auto& object : objects_) {
        j["objects"].push_back(object->toJson());
    }
    
    // Walls
    j["walls"] = json::array();
    for (const auto& wall : walls_) {
        json wallJson;
        wallJson["id"] = wall.id;
        wallJson["start"] = {{"x", wall.start.x}, {"y", wall.start.y}, {"z", wall.start.z}};
        wallJson["end"] = {{"x", wall.end.x}, {"y", wall.end.y}, {"z", wall.end.z}};
        wallJson["height"] = wall.height;
        wallJson["thickness"] = wall.thickness;
        wallJson["materialId"] = wall.materialId;
        j["walls"].push_back(wallJson);
    }
    
    // Openings
    j["openings"] = json::array();
    for (const auto& opening : openings_) {
        json openingJson;
        openingJson["id"] = opening.id;
        openingJson["wallId"] = opening.wallId;
        openingJson["type"] = opening.type;
        openingJson["position"] = opening.position;
        openingJson["width"] = opening.width;
        openingJson["height"] = opening.height;
        openingJson["sillHeight"] = opening.sillHeight;
        j["openings"].push_back(openingJson);
    }
    
    return j;
}

void Project::fromJson(const json& j) {
    if (j.contains("id")) id_ = j["id"];
    if (j.contains("name")) name_ = j["name"];
    if (j.contains("description")) description_ = j["description"];
    if (j.contains("thumbnailPath")) thumbnailPath_ = j["thumbnailPath"];
    
    // Dimensions
    if (j.contains("dimensions")) {
        const auto& dims = j["dimensions"];
        dimensions_ = RoomDimensions(dims["width"], dims["height"], dims["depth"]);
    }
    
    // Timestamps (simplified parsing)
    if (j.contains("createdAt")) {
        // In a full implementation, would parse the timestamp properly
        createdAt_ = std::chrono::system_clock::now();
    }
    if (j.contains("updatedAt")) {
        updatedAt_ = std::chrono::system_clock::now();
    }
    
    // Objects
    objects_.clear();
    if (j.contains("objects")) {
        for (const auto& objJson : j["objects"]) {
            auto object = std::make_unique<SceneObject>();
            object->fromJson(objJson);
            objects_.push_back(std::move(object));
        }
    }
    
    // Walls
    walls_.clear();
    if (j.contains("walls")) {
        for (const auto& wallJson : j["walls"]) {
            Wall wall;
            wall.id = wallJson["id"];
            wall.start = Point3D(wallJson["start"]["x"], wallJson["start"]["y"], wallJson["start"]["z"]);
            wall.end = Point3D(wallJson["end"]["x"], wallJson["end"]["y"], wallJson["end"]["z"]);
            wall.height = wallJson["height"];
            wall.thickness = wallJson["thickness"];
            if (wallJson.contains("materialId")) {
                wall.materialId = wallJson["materialId"];
            }
            walls_.push_back(wall);
        }
    }
    
    // Openings
    openings_.clear();
    if (j.contains("openings")) {
        for (const auto& openingJson : j["openings"]) {
            Opening opening;
            opening.id = openingJson["id"];
            opening.wallId = openingJson["wallId"];
            opening.type = openingJson["type"];
            opening.position = openingJson["position"];
            opening.width = openingJson["width"];
            opening.height = openingJson["height"];
            opening.sillHeight = openingJson["sillHeight"];
            openings_.push_back(opening);
        }
    }
}

json Project::serializeSceneToJson() const {
    json sceneJson;
    
    // Objects
    sceneJson["objects"] = json::array();
    for (const auto& object : objects_) {
        sceneJson["objects"].push_back(object->toJson());
    }
    
    // Walls
    sceneJson["walls"] = json::array();
    for (const auto& wall : walls_) {
        json wallJson;
        wallJson["id"] = wall.id;
        wallJson["start"] = {{"x", wall.start.x}, {"y", wall.start.y}, {"z", wall.start.z}};
        wallJson["end"] = {{"x", wall.end.x}, {"y", wall.end.y}, {"z", wall.end.z}};
        wallJson["height"] = wall.height;
        wallJson["thickness"] = wall.thickness;
        wallJson["materialId"] = wall.materialId;
        sceneJson["walls"].push_back(wallJson);
    }
    
    // Openings
    sceneJson["openings"] = json::array();
    for (const auto& opening : openings_) {
        json openingJson;
        openingJson["id"] = opening.id;
        openingJson["wallId"] = opening.wallId;
        openingJson["type"] = opening.type;
        openingJson["position"] = opening.position;
        openingJson["width"] = opening.width;
        openingJson["height"] = opening.height;
        openingJson["sillHeight"] = opening.sillHeight;
        sceneJson["openings"].push_back(openingJson);
    }
    
    return sceneJson;
}

void Project::loadSceneFromJson(const json& sceneJson) {
    // Clear existing scene data
    objects_.clear();
    walls_.clear();
    openings_.clear();
    
    // Load objects
    if (sceneJson.contains("objects")) {
        for (const auto& objJson : sceneJson["objects"]) {
            auto object = std::make_unique<SceneObject>();
            object->fromJson(objJson);
            objects_.push_back(std::move(object));
        }
    }
    
    // Load walls
    if (sceneJson.contains("walls")) {
        for (const auto& wallJson : sceneJson["walls"]) {
            Wall wall;
            wall.id = wallJson["id"];
            wall.start = Point3D(wallJson["start"]["x"], wallJson["start"]["y"], wallJson["start"]["z"]);
            wall.end = Point3D(wallJson["end"]["x"], wallJson["end"]["y"], wallJson["end"]["z"]);
            wall.height = wallJson["height"];
            wall.thickness = wallJson["thickness"];
            if (wallJson.contains("materialId")) {
                wall.materialId = wallJson["materialId"];
            }
            walls_.push_back(wall);
        }
    }
    
    // Load openings
    if (sceneJson.contains("openings")) {
        for (const auto& openingJson : sceneJson["openings"]) {
            Opening opening;
            opening.id = openingJson["id"];
            opening.wallId = openingJson["wallId"];
            opening.type = openingJson["type"];
            opening.position = openingJson["position"];
            opening.width = openingJson["width"];
            opening.height = openingJson["height"];
            opening.sillHeight = openingJson["sillHeight"];
            openings_.push_back(opening);
        }
    }
    
    updateTimestamp();
}

ProjectInfo Project::toProjectInfo() const {
    ProjectInfo info;
    info.id = id_;
    info.name = name_;
    info.description = description_;
    info.roomWidth = dimensions_.width;
    info.roomHeight = dimensions_.height;
    info.roomDepth = dimensions_.depth;
    info.objectCount = objects_.size();
    info.thumbnailPath = thumbnailPath_;
    
    // Format timestamps
    auto createdTime = std::chrono::system_clock::to_time_t(createdAt_);
    auto updatedTime = std::chrono::system_clock::to_time_t(updatedAt_);
    
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&createdTime), "%Y-%m-%d %H:%M:%S");
    info.createdAt = ss.str();
    
    ss.str("");
    ss << std::put_time(std::gmtime(&updatedTime), "%Y-%m-%d %H:%M:%S");
    info.updatedAt = ss.str();
    
    return info;
}

ProjectMetadata Project::toProjectMetadata() const {
    ProjectMetadata metadata;
    metadata.name = name_;
    metadata.description = description_;
    metadata.roomWidth = dimensions_.width;
    metadata.roomHeight = dimensions_.height;
    metadata.roomDepth = dimensions_.depth;
    
    return metadata;
}

void Project::updateFromMetadata(const ProjectMetadata& metadata) {
    name_ = metadata.name;
    description_ = metadata.description;
    dimensions_ = RoomDimensions(metadata.roomWidth, metadata.roomHeight, metadata.roomDepth);
    updateTimestamp();
}

std::string Project::generateId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    ss << "proj_";
    for (int i = 0; i < 12; ++i) {
        ss << std::hex << dis(gen);
    }
    return ss.str();
}

void Project::initializeTimestamps() {
    auto now = std::chrono::system_clock::now();
    createdAt_ = now;
    updatedAt_ = now;
}

} // namespace Models
} // namespace KitchenCAD