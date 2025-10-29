#include "SceneManager.h"
#include "../utils/Logger.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <chrono>

namespace KitchenCAD {
namespace Scene {

// SpatialIndex Implementation

SpatialIndex::SpatialIndex(double cellSize) : cellSize_(cellSize) {
    if (cellSize <= 0.0) {
        cellSize_ = 1.0;
        LOG_WARNING("Invalid cell size provided, using default value of 1.0");
    }
}

void SpatialIndex::addObject(const ObjectId& id, const Geometry::BoundingBox& bounds) {
    if (bounds.isEmpty()) return;
    
    auto cells = getCellsForBounds(bounds);
    for (const auto& cellKey : cells) {
        grid_[cellKey].objects.insert(id);
    }
}

void SpatialIndex::removeObject(const ObjectId& id, const Geometry::BoundingBox& bounds) {
    if (bounds.isEmpty()) return;
    
    auto cells = getCellsForBounds(bounds);
    for (const auto& cellKey : cells) {
        auto it = grid_.find(cellKey);
        if (it != grid_.end()) {
            it->second.objects.erase(id);
            if (it->second.objects.empty()) {
                grid_.erase(it);
            }
        }
    }
}

void SpatialIndex::updateObject(const ObjectId& id, const Geometry::BoundingBox& oldBounds, 
                               const Geometry::BoundingBox& newBounds) {
    removeObject(id, oldBounds);
    addObject(id, newBounds);
}

std::vector<ObjectId> SpatialIndex::queryRegion(const Geometry::BoundingBox& region) const {
    std::unordered_set<ObjectId> result;
    
    auto cells = getCellsForBounds(region);
    for (const auto& cellKey : cells) {
        auto it = grid_.find(cellKey);
        if (it != grid_.end()) {
            for (const auto& objectId : it->second.objects) {
                result.insert(objectId);
            }
        }
    }
    
    return std::vector<ObjectId>(result.begin(), result.end());
}

std::vector<ObjectId> SpatialIndex::queryRadius(const Geometry::Point3D& center, double radius) const {
    Geometry::BoundingBox region(
        Geometry::Point3D(center.x - radius, center.y - radius, center.z - radius),
        Geometry::Point3D(center.x + radius, center.y + radius, center.z + radius)
    );
    return queryRegion(region);
}

void SpatialIndex::clear() {
    grid_.clear();
}

std::string SpatialIndex::getCellKey(int x, int y, int z) const {
    std::ostringstream oss;
    oss << x << "," << y << "," << z;
    return oss.str();
}

std::vector<std::string> SpatialIndex::getCellsForBounds(const Geometry::BoundingBox& bounds) const {
    std::vector<std::string> cells;
    
    if (bounds.isEmpty()) return cells;
    
    int minX = static_cast<int>(std::floor(bounds.min.x / cellSize_));
    int maxX = static_cast<int>(std::floor(bounds.max.x / cellSize_));
    int minY = static_cast<int>(std::floor(bounds.min.y / cellSize_));
    int maxY = static_cast<int>(std::floor(bounds.max.y / cellSize_));
    int minZ = static_cast<int>(std::floor(bounds.min.z / cellSize_));
    int maxZ = static_cast<int>(std::floor(bounds.max.z / cellSize_));
    
    for (int x = minX; x <= maxX; ++x) {
        for (int y = minY; y <= maxY; ++y) {
            for (int z = minZ; z <= maxZ; ++z) {
                cells.push_back(getCellKey(x, y, z));
            }
        }
    }
    
    return cells;
}

// CollisionDetector Implementation

bool CollisionDetector::checkBoundingBoxIntersection(const Geometry::BoundingBox& a, 
                                                    const Geometry::BoundingBox& b) {
    return a.intersects(b);
}

CollisionDetector::CollisionInfo CollisionDetector::calculatePenetration(
    const ObjectId& idA, const ObjectId& idB,
    const Geometry::BoundingBox& a, const Geometry::BoundingBox& b) {
    
    // Calculate overlap in each dimension
    double overlapX = std::min(a.max.x, b.max.x) - std::max(a.min.x, b.min.x);
    double overlapY = std::min(a.max.y, b.max.y) - std::max(a.min.y, b.min.y);
    double overlapZ = std::min(a.max.z, b.max.z) - std::max(a.min.z, b.min.z);
    
    // Find the axis with minimum overlap (separation axis)
    double minOverlap = std::min({overlapX, overlapY, overlapZ});
    Geometry::Vector3D penetrationVector;
    
    if (minOverlap == overlapX) {
        penetrationVector = Geometry::Vector3D(
            (a.center().x < b.center().x) ? -overlapX : overlapX, 0, 0);
    } else if (minOverlap == overlapY) {
        penetrationVector = Geometry::Vector3D(
            0, (a.center().y < b.center().y) ? -overlapY : overlapY, 0);
    } else {
        penetrationVector = Geometry::Vector3D(
            0, 0, (a.center().z < b.center().z) ? -overlapZ : overlapZ);
    }
    
    return CollisionInfo(idA, idB, penetrationVector, minOverlap);
}

bool CollisionDetector::wouldCollide(const Geometry::BoundingBox& objectBounds,
                                    const Geometry::Transform3D& transform,
                                    const std::vector<Geometry::BoundingBox>& otherBounds) {
    
    Geometry::BoundingBox transformedBounds = objectBounds.transformed(transform);
    
    for (const auto& otherBox : otherBounds) {
        if (checkBoundingBoxIntersection(transformedBounds, otherBox)) {
            return true;
        }
    }
    
    return false;
}

// SceneManager Implementation

SceneManager::SceneManager(double spatialCellSize, double collisionTolerance)
    : spatialIndex_(std::make_unique<SpatialIndex>(spatialCellSize))
    , randomGenerator_(std::chrono::steady_clock::now().time_since_epoch().count())
    , idDistribution_(0, std::numeric_limits<uint64_t>::max())
    , collisionTolerance_(collisionTolerance)
    , enableCollisionDetection_(true) {
    
    LOG_INFO("SceneManager initialized with spatial cell size: " + std::to_string(spatialCellSize) +
             ", collision tolerance: " + std::to_string(collisionTolerance));
}

ObjectId SceneManager::addObject(std::unique_ptr<SceneObject> object) {
    if (!object) {
        LOG_ERROR("Cannot add null object to scene");
        return "";
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    ObjectId id = object->getId();
    if (id.empty()) {
        id = generateUniqueId();
        object->setId(id);
    }
    
    // Check if ID already exists
    if (objects_.find(id) != objects_.end()) {
        LOG_WARNING("Object with ID " + id + " already exists, generating new ID");
        id = generateUniqueId();
        object->setId(id);
    }
    
    // Calculate bounding box
    Geometry::BoundingBox bounds = calculateObjectBounds(*object);
    
    // Store object and its bounds
    objects_[id] = std::move(object);
    objectBounds_[id] = bounds;
    
    // Add to spatial index
    spatialIndex_->addObject(id, bounds);
    
    LOG_DEBUG("Added object " + id + " to scene");
    notifyObjectAdded(id);
    
    return id;
}

bool SceneManager::removeObject(const ObjectId& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = objects_.find(id);
    if (it == objects_.end()) {
        LOG_WARNING("Attempted to remove non-existent object: " + id);
        return false;
    }
    
    // Remove from spatial index
    auto boundsIt = objectBounds_.find(id);
    if (boundsIt != objectBounds_.end()) {
        spatialIndex_->removeObject(id, boundsIt->second);
        objectBounds_.erase(boundsIt);
    }
    
    // Remove from selection if selected
    selectedObjects_.erase(id);
    
    // Remove object
    objects_.erase(it);
    
    LOG_DEBUG("Removed object " + id + " from scene");
    notifyObjectRemoved(id);
    
    return true;
}

SceneObject* SceneManager::getObject(const ObjectId& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = objects_.find(id);
    return (it != objects_.end()) ? it->second.get() : nullptr;
}

const SceneObject* SceneManager::getObject(const ObjectId& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = objects_.find(id);
    return (it != objects_.end()) ? it->second.get() : nullptr;
}

std::vector<ObjectId> SceneManager::getAllObjects() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<ObjectId> result;
    result.reserve(objects_.size());
    
    for (const auto& pair : objects_) {
        result.push_back(pair.first);
    }
    
    return result;
}

std::vector<ObjectId> SceneManager::getObjectsInRegion(const Geometry::BoundingBox& region) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto candidates = spatialIndex_->queryRegion(region);
    std::vector<ObjectId> result;
    
    for (const auto& id : candidates) {
        auto boundsIt = objectBounds_.find(id);
        if (boundsIt != objectBounds_.end() && region.intersects(boundsIt->second)) {
            result.push_back(id);
        }
    }
    
    return result;
}

std::vector<ObjectId> SceneManager::getObjectsOfType(const std::string& type) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<ObjectId> result;
    
    for (const auto& pair : objects_) {
        // This would need to be implemented based on how SceneObject stores type information
        // For now, we'll use a placeholder implementation
        result.push_back(pair.first);
    }
    
    return result;
}

std::vector<ObjectId> SceneManager::getObjectsByCategory(const std::string& category) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<ObjectId> result;
    
    for (const auto& pair : objects_) {
        // This would need to be implemented based on how SceneObject stores category information
        // For now, we'll use a placeholder implementation
        result.push_back(pair.first);
    }
    
    return result;
}

std::vector<ObjectId> SceneManager::findIntersectingObjects(const ObjectId& objectId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto boundsIt = objectBounds_.find(objectId);
    if (boundsIt == objectBounds_.end()) {
        return {};
    }
    
    const auto& bounds = boundsIt->second;
    auto candidates = spatialIndex_->queryRegion(bounds);
    std::vector<ObjectId> result;
    
    for (const auto& candidateId : candidates) {
        if (candidateId == objectId) continue;
        
        auto candidateBoundsIt = objectBounds_.find(candidateId);
        if (candidateBoundsIt != objectBounds_.end() && 
            CollisionDetector::checkBoundingBoxIntersection(bounds, candidateBoundsIt->second)) {
            result.push_back(candidateId);
        }
    }
    
    return result;
}

std::vector<ObjectId> SceneManager::findNearbyObjects(const ObjectId& objectId, double radius) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto boundsIt = objectBounds_.find(objectId);
    if (boundsIt == objectBounds_.end()) {
        return {};
    }
    
    Geometry::Point3D center = boundsIt->second.center();
    auto candidates = spatialIndex_->queryRadius(center, radius);
    std::vector<ObjectId> result;
    
    for (const auto& candidateId : candidates) {
        if (candidateId == objectId) continue;
        
        auto candidateBoundsIt = objectBounds_.find(candidateId);
        if (candidateBoundsIt != objectBounds_.end()) {
            double distance = center.distanceTo(candidateBoundsIt->second.center());
            if (distance <= radius) {
                result.push_back(candidateId);
            }
        }
    }
    
    return result;
}

bool SceneManager::checkCollision(const ObjectId& objectId, const Geometry::Transform3D& newTransform) const {
    if (!enableCollisionDetection_) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto boundsIt = objectBounds_.find(objectId);
    if (boundsIt == objectBounds_.end()) {
        return false;
    }
    
    // Get all other object bounds
    std::vector<Geometry::BoundingBox> otherBounds;
    for (const auto& pair : objectBounds_) {
        if (pair.first != objectId) {
            otherBounds.push_back(pair.second);
        }
    }
    
    return CollisionDetector::wouldCollide(boundsIt->second, newTransform, otherBounds);
}

bool SceneManager::moveObject(const ObjectId& id, const Geometry::Transform3D& transform) {
    return applyTransformToObject(id, transform);
}

bool SceneManager::translateObject(const ObjectId& id, const Geometry::Vector3D& translation) {
    auto object = getObject(id);
    if (!object) return false;
    
    Geometry::Transform3D currentTransform = object->getTransform();
    currentTransform.translate(translation);
    
    return applyTransformToObject(id, currentTransform);
}

bool SceneManager::rotateObject(const ObjectId& id, const Geometry::Vector3D& rotation) {
    auto object = getObject(id);
    if (!object) return false;
    
    Geometry::Transform3D currentTransform = object->getTransform();
    currentTransform.rotate(rotation);
    
    return applyTransformToObject(id, currentTransform);
}

bool SceneManager::scaleObject(const ObjectId& id, const Geometry::Vector3D& scale) {
    auto object = getObject(id);
    if (!object) return false;
    
    Geometry::Transform3D currentTransform = object->getTransform();
    currentTransform.scaleBy(scale);
    
    return applyTransformToObject(id, currentTransform);
}

void SceneManager::setSelection(const std::vector<ObjectId>& selection) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    selectedObjects_.clear();
    for (const auto& id : selection) {
        if (validateObjectId(id)) {
            selectedObjects_.insert(id);
        }
    }
    
    notifySelectionChanged();
}

void SceneManager::addToSelection(const ObjectId& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (validateObjectId(id)) {
        selectedObjects_.insert(id);
        notifySelectionChanged();
    }
}

void SceneManager::removeFromSelection(const ObjectId& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (selectedObjects_.erase(id) > 0) {
        notifySelectionChanged();
    }
}

void SceneManager::clearSelection() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!selectedObjects_.empty()) {
        selectedObjects_.clear();
        notifySelectionChanged();
    }
}

std::vector<ObjectId> SceneManager::getSelection() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    return std::vector<ObjectId>(selectedObjects_.begin(), selectedObjects_.end());
}

bool SceneManager::isSelected(const ObjectId& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    return selectedObjects_.find(id) != selectedObjects_.end();
}

Geometry::BoundingBox SceneManager::getSceneBounds() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    Geometry::BoundingBox result;
    
    for (const auto& pair : objectBounds_) {
        result.expand(pair.second);
    }
    
    return result;
}

size_t SceneManager::getObjectCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return objects_.size();
}

bool SceneManager::isEmpty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return objects_.empty();
}

void SceneManager::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    objects_.clear();
    objectBounds_.clear();
    selectedObjects_.clear();
    spatialIndex_->clear();
    
    LOG_INFO("Scene cleared");
}

std::unique_ptr<SceneObject> SceneManager::duplicateObject(const ObjectId& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = objects_.find(id);
    if (it == objects_.end()) {
        LOG_WARNING("Cannot duplicate non-existent object: " + id);
        return nullptr;
    }
    
    // Create a copy using JSON serialization/deserialization
    try {
        nlohmann::json objectJson = it->second->toJson();
        auto duplicate = std::make_unique<SceneObject>();
        duplicate->fromJson(objectJson);
        
        // Generate new ID for the duplicate
        duplicate->setId(generateUniqueId());
        
        return duplicate;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to duplicate object " + id + ": " + e.what());
        return nullptr;
    }
}

void SceneManager::forEachObject(std::function<void(const ObjectId&, SceneObject*)> callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& pair : objects_) {
        callback(pair.first, pair.second.get());
    }
}

void SceneManager::forEachObject(std::function<void(const ObjectId&, const SceneObject*)> callback) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (const auto& pair : objects_) {
        callback(pair.first, pair.second.get());
    }
}

void SceneManager::setObjectAddedCallback(ObjectCallback callback) {
    objectAddedCallback_ = callback;
}

void SceneManager::setObjectRemovedCallback(ObjectCallback callback) {
    objectRemovedCallback_ = callback;
}

void SceneManager::setObjectModifiedCallback(ObjectCallback callback) {
    objectModifiedCallback_ = callback;
}

void SceneManager::setSelectionChangedCallback(SelectionCallback callback) {
    selectionChangedCallback_ = callback;
}

std::vector<CollisionDetector::CollisionInfo> SceneManager::detectAllCollisions() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<CollisionDetector::CollisionInfo> collisions;
    
    // Create object IDs list directly without calling getAllObjects() to avoid deadlock
    std::vector<ObjectId> objectIds;
    objectIds.reserve(objects_.size());
    for (const auto& pair : objects_) {
        objectIds.push_back(pair.first);
    }
    
    for (size_t i = 0; i < objectIds.size(); ++i) {
        for (size_t j = i + 1; j < objectIds.size(); ++j) {
            const auto& idA = objectIds[i];
            const auto& idB = objectIds[j];
            
            auto boundsA = objectBounds_.find(idA);
            auto boundsB = objectBounds_.find(idB);
            
            if (boundsA != objectBounds_.end() && boundsB != objectBounds_.end()) {
                if (CollisionDetector::checkBoundingBoxIntersection(boundsA->second, boundsB->second)) {
                    collisions.push_back(
                        CollisionDetector::calculatePenetration(idA, idB, boundsA->second, boundsB->second)
                    );
                }
            }
        }
    }
    
    return collisions;
}

std::vector<ObjectId> SceneManager::getAffectedObjects(const ObjectId& objectId, 
                                                      const Geometry::Transform3D& newTransform) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto boundsIt = objectBounds_.find(objectId);
    if (boundsIt == objectBounds_.end()) {
        return {};
    }
    
    Geometry::BoundingBox transformedBounds = boundsIt->second.transformed(newTransform);
    return getObjectsInRegion(transformedBounds);
}

std::vector<std::string> SceneManager::validateScene() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> issues;
    
    // Check for objects without valid bounds
    for (const auto& pair : objects_) {
        auto boundsIt = objectBounds_.find(pair.first);
        if (boundsIt == objectBounds_.end() || boundsIt->second.isEmpty()) {
            issues.push_back("Object " + pair.first + " has invalid bounding box");
        }
    }
    
    // Check for selected objects that don't exist
    for (const auto& selectedId : selectedObjects_) {
        if (objects_.find(selectedId) == objects_.end()) {
            issues.push_back("Selected object " + selectedId + " does not exist");
        }
    }
    
    return issues;
}

SceneManager::SceneStatistics SceneManager::getStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    SceneStatistics stats;
    stats.totalObjects = objects_.size();
    stats.selectedObjects = selectedObjects_.size();
    
    // Calculate collisions directly to avoid deadlock
    std::vector<ObjectId> objectIds;
    objectIds.reserve(objects_.size());
    for (const auto& pair : objects_) {
        objectIds.push_back(pair.first);
    }
    
    size_t collisionCount = 0;
    for (size_t i = 0; i < objectIds.size(); ++i) {
        for (size_t j = i + 1; j < objectIds.size(); ++j) {
            const auto& idA = objectIds[i];
            const auto& idB = objectIds[j];
            
            auto boundsA = objectBounds_.find(idA);
            auto boundsB = objectBounds_.find(idB);
            
            if (boundsA != objectBounds_.end() && boundsB != objectBounds_.end()) {
                if (CollisionDetector::checkBoundingBoxIntersection(boundsA->second, boundsB->second)) {
                    collisionCount++;
                }
            }
        }
    }
    stats.collisions = collisionCount;
    
    // Calculate scene bounds directly to avoid deadlock
    Geometry::BoundingBox sceneBounds;
    for (const auto& pair : objectBounds_) {
        sceneBounds.expand(pair.second);
    }
    stats.sceneBounds = sceneBounds;
    
    // Calculate total volume
    stats.totalVolume = 0.0;
    for (const auto& pair : objectBounds_) {
        stats.totalVolume += pair.second.volume();
    }
    
    return stats;
}

// Private methods

ObjectId SceneManager::generateUniqueId() {
    uint64_t id = idDistribution_(randomGenerator_);
    
    std::ostringstream oss;
    oss << "obj_" << std::hex << id;
    
    // Ensure uniqueness
    std::string idStr = oss.str();
    while (objects_.find(idStr) != objects_.end()) {
        id = idDistribution_(randomGenerator_);
        oss.str("");
        oss << "obj_" << std::hex << id;
        idStr = oss.str();
    }
    
    return idStr;
}

Geometry::BoundingBox SceneManager::calculateObjectBounds(const SceneObject& object) const {
    // For now, create a simple bounding box based on the object's transform
    // In a real implementation, this would use the object's geometry
    const auto& transform = object.getTransform();
    
    // Create a unit cube and transform it
    Geometry::BoundingBox unitBox(
        Geometry::Point3D(-0.5, -0.5, -0.5),
        Geometry::Point3D(0.5, 0.5, 0.5)
    );
    
    return unitBox.transformed(transform);
}

void SceneManager::updateSpatialIndex(const ObjectId& id, const Geometry::BoundingBox& oldBounds, 
                                     const Geometry::BoundingBox& newBounds) {
    spatialIndex_->updateObject(id, oldBounds, newBounds);
}

void SceneManager::notifyObjectAdded(const ObjectId& id) {
    if (objectAddedCallback_) {
        objectAddedCallback_(id);
    }
}

void SceneManager::notifyObjectRemoved(const ObjectId& id) {
    if (objectRemovedCallback_) {
        objectRemovedCallback_(id);
    }
}

void SceneManager::notifyObjectModified(const ObjectId& id) {
    if (objectModifiedCallback_) {
        objectModifiedCallback_(id);
    }
}

void SceneManager::notifySelectionChanged() {
    if (selectionChangedCallback_) {
        std::vector<ObjectId> selection(selectedObjects_.begin(), selectedObjects_.end());
        selectionChangedCallback_(selection);
    }
}

bool SceneManager::validateObjectId(const ObjectId& id) const {
    return objects_.find(id) != objects_.end();
}

bool SceneManager::applyTransformToObject(const ObjectId& id, const Geometry::Transform3D& transform) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto objectIt = objects_.find(id);
    if (objectIt == objects_.end()) {
        LOG_WARNING("Cannot transform non-existent object: " + id);
        return false;
    }
    
    auto boundsIt = objectBounds_.find(id);
    if (boundsIt == objectBounds_.end()) {
        LOG_ERROR("Object " + id + " has no bounding box information");
        return false;
    }
    
    // Check for collisions if enabled
    if (enableCollisionDetection_) {
        // Get all other object bounds (without acquiring lock since we already have it)
        std::vector<Geometry::BoundingBox> otherBounds;
        for (const auto& pair : objectBounds_) {
            if (pair.first != id) {
                otherBounds.push_back(pair.second);
            }
        }
        
        if (CollisionDetector::wouldCollide(boundsIt->second, transform, otherBounds)) {
            LOG_DEBUG("Transform rejected due to collision for object: " + id);
            return false;
        }
    }
    
    // Store old bounds for spatial index update
    Geometry::BoundingBox oldBounds = boundsIt->second;
    
    // Apply transform to object
    objectIt->second->setTransform(transform);
    
    // Recalculate bounds
    Geometry::BoundingBox newBounds = calculateObjectBounds(*objectIt->second);
    boundsIt->second = newBounds;
    
    // Update spatial index
    updateSpatialIndex(id, oldBounds, newBounds);
    
    LOG_DEBUG("Applied transform to object: " + id);
    notifyObjectModified(id);
    
    return true;
}

} // namespace Scene
} // namespace KitchenCAD