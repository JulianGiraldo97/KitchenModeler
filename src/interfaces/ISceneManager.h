#pragma once

#include "../geometry/Point3D.h"
#include "../geometry/Vector3D.h"
#include "../geometry/BoundingBox.h"
#include "../geometry/Transform3D.h"
#include <memory>
#include <vector>
#include <string>
#include <functional>

namespace KitchenCAD {

// Forward declarations
namespace Models {
    class SceneObject;
}
using SceneObject = Models::SceneObject;

/**
 * @brief Unique identifier for scene objects
 */
using ObjectId = std::string;

/**
 * @brief Interface for managing objects in the 3D scene
 * 
 * This interface defines the contract for scene management including
 * object addition, removal, selection, and spatial queries.
 */
class ISceneManager {
public:
    virtual ~ISceneManager() = default;
    
    // Object management
    virtual ObjectId addObject(std::unique_ptr<SceneObject> object) = 0;
    virtual bool removeObject(const ObjectId& id) = 0;
    virtual SceneObject* getObject(const ObjectId& id) = 0;
    virtual const SceneObject* getObject(const ObjectId& id) const = 0;
    
    // Object queries
    virtual std::vector<ObjectId> getAllObjects() const = 0;
    virtual std::vector<ObjectId> getObjectsInRegion(const Geometry::BoundingBox& region) const = 0;
    virtual std::vector<ObjectId> getObjectsOfType(const std::string& type) const = 0;
    virtual std::vector<ObjectId> getObjectsByCategory(const std::string& category) const = 0;
    
    // Spatial queries
    virtual std::vector<ObjectId> findIntersectingObjects(const ObjectId& objectId) const = 0;
    virtual std::vector<ObjectId> findNearbyObjects(const ObjectId& objectId, double radius) const = 0;
    virtual bool checkCollision(const ObjectId& objectId, const Geometry::Transform3D& newTransform) const = 0;
    
    // Object transformation
    virtual bool moveObject(const ObjectId& id, const Geometry::Transform3D& transform) = 0;
    virtual bool translateObject(const ObjectId& id, const Geometry::Vector3D& translation) = 0;
    virtual bool rotateObject(const ObjectId& id, const Geometry::Vector3D& rotation) = 0;
    virtual bool scaleObject(const ObjectId& id, const Geometry::Vector3D& scale) = 0;
    
    // Selection management
    virtual void setSelection(const std::vector<ObjectId>& selection) = 0;
    virtual void addToSelection(const ObjectId& id) = 0;
    virtual void removeFromSelection(const ObjectId& id) = 0;
    virtual void clearSelection() = 0;
    virtual std::vector<ObjectId> getSelection() const = 0;
    virtual bool isSelected(const ObjectId& id) const = 0;
    
    // Scene properties
    virtual Geometry::BoundingBox getSceneBounds() const = 0;
    virtual size_t getObjectCount() const = 0;
    virtual bool isEmpty() const = 0;
    
    // Scene operations
    virtual void clear() = 0;
    virtual std::unique_ptr<SceneObject> duplicateObject(const ObjectId& id) = 0;
    
    // Iteration support
    virtual void forEachObject(std::function<void(const ObjectId&, SceneObject*)> callback) = 0;
    virtual void forEachObject(std::function<void(const ObjectId&, const SceneObject*)> callback) const = 0;
    
    // Event callbacks (for notifications)
    using ObjectCallback = std::function<void(const ObjectId&)>;
    using SelectionCallback = std::function<void(const std::vector<ObjectId>&)>;
    
    virtual void setObjectAddedCallback(ObjectCallback callback) = 0;
    virtual void setObjectRemovedCallback(ObjectCallback callback) = 0;
    virtual void setObjectModifiedCallback(ObjectCallback callback) = 0;
    virtual void setSelectionChangedCallback(SelectionCallback callback) = 0;
};

} // namespace KitchenCAD