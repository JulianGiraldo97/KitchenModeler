#pragma once

#include "../interfaces/ISceneManager.h"
#include "../models/Project.h"
#include "../geometry/BoundingBox.h"
#include "../geometry/Transform3D.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <string>
#include <functional>
#include <mutex>
#include <random>

namespace KitchenCAD {
namespace Scene {

/**
 * @brief Spatial index for efficient spatial queries
 * 
 * Simple spatial partitioning system for fast collision detection
 * and spatial queries. Uses a grid-based approach for simplicity.
 */
class SpatialIndex {
private:
    struct GridCell {
        std::unordered_set<ObjectId> objects;
    };
    
    double cellSize_;
    std::unordered_map<std::string, GridCell> grid_;
    
public:
    explicit SpatialIndex(double cellSize = 1.0);
    
    void addObject(const ObjectId& id, const Geometry::BoundingBox& bounds);
    void removeObject(const ObjectId& id, const Geometry::BoundingBox& bounds);
    void updateObject(const ObjectId& id, const Geometry::BoundingBox& oldBounds, 
                      const Geometry::BoundingBox& newBounds);
    
    std::vector<ObjectId> queryRegion(const Geometry::BoundingBox& region) const;
    std::vector<ObjectId> queryRadius(const Geometry::Point3D& center, double radius) const;
    
    void clear();
    
private:
    std::string getCellKey(int x, int y, int z) const;
    std::vector<std::string> getCellsForBounds(const Geometry::BoundingBox& bounds) const;
};

/**
 * @brief Collision detection system
 * 
 * Handles collision detection between objects using bounding boxes
 * and more precise geometric tests when available.
 */
class CollisionDetector {
public:
    struct CollisionInfo {
        ObjectId objectA;
        ObjectId objectB;
        Geometry::Vector3D penetrationVector;
        double penetrationDepth;
        
        CollisionInfo(const ObjectId& a, const ObjectId& b, 
                     const Geometry::Vector3D& vector, double depth)
            : objectA(a), objectB(b), penetrationVector(vector), penetrationDepth(depth) {}
    };
    
    /**
     * @brief Check if two bounding boxes intersect
     */
    static bool checkBoundingBoxIntersection(const Geometry::BoundingBox& a, 
                                           const Geometry::BoundingBox& b);
    
    /**
     * @brief Calculate penetration vector between two intersecting boxes
     */
    static CollisionInfo calculatePenetration(const ObjectId& idA, const ObjectId& idB,
                                            const Geometry::BoundingBox& a, 
                                            const Geometry::BoundingBox& b);
    
    /**
     * @brief Check if an object at a given transform would collide with others
     */
    static bool wouldCollide(const Geometry::BoundingBox& objectBounds,
                           const Geometry::Transform3D& transform,
                           const std::vector<Geometry::BoundingBox>& otherBounds);
};

/**
 * @brief Main scene manager implementation
 * 
 * Manages all objects in the 3D scene, handles selection, transformations,
 * collision detection, and spatial queries.
 */
class SceneManager : public ISceneManager {
private:
    // Object storage
    std::unordered_map<ObjectId, std::unique_ptr<SceneObject>> objects_;
    std::unordered_map<ObjectId, Geometry::BoundingBox> objectBounds_;
    
    // Selection management
    std::unordered_set<ObjectId> selectedObjects_;
    
    // Spatial indexing
    std::unique_ptr<SpatialIndex> spatialIndex_;
    
    // ID generation
    std::mt19937 randomGenerator_;
    std::uniform_int_distribution<uint64_t> idDistribution_;
    
    // Thread safety
    mutable std::mutex mutex_;
    
    // Event callbacks
    ObjectCallback objectAddedCallback_;
    ObjectCallback objectRemovedCallback_;
    ObjectCallback objectModifiedCallback_;
    SelectionCallback selectionChangedCallback_;
    
    // Configuration
    double collisionTolerance_;
    bool enableCollisionDetection_;
    
public:
    /**
     * @brief Constructor
     */
    explicit SceneManager(double spatialCellSize = 1.0, double collisionTolerance = 1e-6);
    
    /**
     * @brief Destructor
     */
    virtual ~SceneManager() = default;
    
    // ISceneManager interface implementation
    
    // Object management
    ObjectId addObject(std::unique_ptr<SceneObject> object) override;
    bool removeObject(const ObjectId& id) override;
    SceneObject* getObject(const ObjectId& id) override;
    const SceneObject* getObject(const ObjectId& id) const override;
    
    // Object queries
    std::vector<ObjectId> getAllObjects() const override;
    std::vector<ObjectId> getObjectsInRegion(const Geometry::BoundingBox& region) const override;
    std::vector<ObjectId> getObjectsOfType(const std::string& type) const override;
    std::vector<ObjectId> getObjectsByCategory(const std::string& category) const override;
    
    // Spatial queries
    std::vector<ObjectId> findIntersectingObjects(const ObjectId& objectId) const override;
    std::vector<ObjectId> findNearbyObjects(const ObjectId& objectId, double radius) const override;
    bool checkCollision(const ObjectId& objectId, const Geometry::Transform3D& newTransform) const override;
    
    // Object transformation
    bool moveObject(const ObjectId& id, const Geometry::Transform3D& transform) override;
    bool translateObject(const ObjectId& id, const Geometry::Vector3D& translation) override;
    bool rotateObject(const ObjectId& id, const Geometry::Vector3D& rotation) override;
    bool scaleObject(const ObjectId& id, const Geometry::Vector3D& scale) override;
    
    // Selection management
    void setSelection(const std::vector<ObjectId>& selection) override;
    void addToSelection(const ObjectId& id) override;
    void removeFromSelection(const ObjectId& id) override;
    void clearSelection() override;
    std::vector<ObjectId> getSelection() const override;
    bool isSelected(const ObjectId& id) const override;
    
    // Scene properties
    Geometry::BoundingBox getSceneBounds() const override;
    size_t getObjectCount() const override;
    bool isEmpty() const override;
    
    // Scene operations
    void clear() override;
    std::unique_ptr<SceneObject> duplicateObject(const ObjectId& id) override;
    
    // Iteration support
    void forEachObject(std::function<void(const ObjectId&, SceneObject*)> callback) override;
    void forEachObject(std::function<void(const ObjectId&, const SceneObject*)> callback) const override;
    
    // Event callbacks
    void setObjectAddedCallback(ObjectCallback callback) override;
    void setObjectRemovedCallback(ObjectCallback callback) override;
    void setObjectModifiedCallback(ObjectCallback callback) override;
    void setSelectionChangedCallback(SelectionCallback callback) override;
    
    // Additional functionality
    
    /**
     * @brief Enable or disable collision detection
     */
    void setCollisionDetectionEnabled(bool enabled) { enableCollisionDetection_ = enabled; }
    
    /**
     * @brief Get collision detection status
     */
    bool isCollisionDetectionEnabled() const { return enableCollisionDetection_; }
    
    /**
     * @brief Set collision tolerance
     */
    void setCollisionTolerance(double tolerance) { collisionTolerance_ = tolerance; }
    
    /**
     * @brief Get collision tolerance
     */
    double getCollisionTolerance() const { return collisionTolerance_; }
    
    /**
     * @brief Get all current collisions in the scene
     */
    std::vector<CollisionDetector::CollisionInfo> detectAllCollisions() const;
    
    /**
     * @brief Get objects that would be affected by moving an object
     */
    std::vector<ObjectId> getAffectedObjects(const ObjectId& objectId, 
                                           const Geometry::Transform3D& newTransform) const;
    
    /**
     * @brief Validate scene integrity (check for invalid objects, etc.)
     */
    std::vector<std::string> validateScene() const;
    
    /**
     * @brief Get statistics about the scene
     */
    struct SceneStatistics {
        size_t totalObjects;
        size_t selectedObjects;
        size_t collisions;
        Geometry::BoundingBox sceneBounds;
        double totalVolume;
        std::unordered_map<std::string, size_t> objectsByType;
    };
    
    SceneStatistics getStatistics() const;

private:
    /**
     * @brief Generate a unique object ID
     */
    ObjectId generateUniqueId();
    
    /**
     * @brief Calculate bounding box for an object
     */
    Geometry::BoundingBox calculateObjectBounds(const SceneObject& object) const;
    
    /**
     * @brief Update spatial index when object changes
     */
    void updateSpatialIndex(const ObjectId& id, const Geometry::BoundingBox& oldBounds, 
                           const Geometry::BoundingBox& newBounds);
    
    /**
     * @brief Notify callbacks about object changes
     */
    void notifyObjectAdded(const ObjectId& id);
    void notifyObjectRemoved(const ObjectId& id);
    void notifyObjectModified(const ObjectId& id);
    void notifySelectionChanged();
    
    /**
     * @brief Validate object ID exists
     */
    bool validateObjectId(const ObjectId& id) const;
    
    /**
     * @brief Apply transform to object and update indices
     */
    bool applyTransformToObject(const ObjectId& id, const Geometry::Transform3D& transform);
};

} // namespace Scene
} // namespace KitchenCAD