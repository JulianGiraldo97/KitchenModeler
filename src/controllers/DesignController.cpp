#include "DesignController.h"
#include "../utils/Logger.h"
#include <algorithm>
#include <cmath>
#include <set>

namespace KitchenCAD {
namespace Controllers {

DesignController::DesignController(std::unique_ptr<ISceneManager> sceneManager,
                                 std::unique_ptr<IGeometryEngine> geometryEngine,
                                 std::unique_ptr<IValidationService> validationService)
    : sceneManager_(std::move(sceneManager))
    , geometryEngine_(std::move(geometryEngine))
    , validationService_(std::move(validationService))
    , currentProject_(nullptr)
    , currentHistoryIndex_(0)
    , maxHistorySize_(100)
{
    // Set up scene manager callbacks
    if (sceneManager_) {
        sceneManager_->setObjectAddedCallback([this](const ObjectId& id) {
            notifyObjectAdded(id);
        });
        
        sceneManager_->setObjectRemovedCallback([this](const ObjectId& id) {
            notifyObjectRemoved(id);
        });
        
        sceneManager_->setObjectModifiedCallback([this](const ObjectId& id) {
            notifyObjectModified(id);
        });
        
        sceneManager_->setSelectionChangedCallback([this](const std::vector<ObjectId>& selection) {
            selectedObjects_ = selection;
            notifySelectionChanged();
        });
    }
}

void DesignController::setCurrentProject(Project* project) {
    currentProject_ = project;
    
    if (sceneManager_) {
        sceneManager_->clear();
        
        // Load project objects into scene manager
        if (project) {
            for (const auto& object : project->getObjects()) {
                // Create a copy for the scene manager
                auto objectCopy = std::make_unique<SceneObject>(*object);
                sceneManager_->addObject(std::move(objectCopy));
            }
        }
    }
    
    clearSelection();
    clearHistory();
}

std::string DesignController::addObjectFromCatalog(const std::string& catalogItemId, const Point3D& position) {
    Transform3D transform;
    transform.translation = position;
    return addObjectFromCatalog(catalogItemId, transform);
}

std::string DesignController::addObjectFromCatalog(const std::string& catalogItemId, const Transform3D& transform) {
    if (!currentProject_ || !sceneManager_) {
        notifyError("No project loaded or scene manager not available");
        return "";
    }
    
    // Apply snapping to position
    Transform3D snappedTransform = transform;
    snappedTransform.translation = applySnapping(transform.translation);
    
    // Validate placement
    if (!isValidPosition(catalogItemId, snappedTransform)) {
        notifyError("Invalid position for object placement");
        return "";
    }
    
    // Create scene object
    auto sceneObject = std::make_unique<SceneObject>(catalogItemId);
    sceneObject->setTransform(snappedTransform);
    
    // Record operation for undo
    DesignOperationRecord record(DesignOperation::AddObject, sceneObject->getId());
    record.afterState = sceneObject->toJson();
    
    // Add to project and scene
    std::string objectId = currentProject_->addObject(std::make_unique<SceneObject>(*sceneObject));
    sceneManager_->addObject(std::move(sceneObject));
    
    recordOperation(record);
    
    return objectId;
}

bool DesignController::removeObject(const std::string& objectId) {
    if (!currentProject_ || !sceneManager_) {
        notifyError("No project loaded or scene manager not available");
        return false;
    }
    
    // Get object for undo record
    const SceneObject* object = currentProject_->getObject(objectId);
    if (!object) {
        notifyError("Object not found: " + objectId);
        return false;
    }
    
    // Record operation for undo
    DesignOperationRecord record(DesignOperation::RemoveObject, objectId);
    record.beforeState = object->toJson();
    
    // Remove from selection if selected
    removeFromSelection(objectId);
    
    // Remove from project and scene
    bool success = currentProject_->removeObject(objectId) && sceneManager_->removeObject(objectId);
    
    if (success) {
        recordOperation(record);
    }
    
    return success;
}

bool DesignController::moveObject(const std::string& objectId, const Point3D& newPosition) {
    Vector3D translation = newPosition - getCurrentObjectPosition(objectId);
    return translateObject(objectId, translation);
}

bool DesignController::translateObject(const std::string& objectId, const Vector3D& translation) {
    if (!currentProject_ || !sceneManager_) {
        notifyError("No project loaded or scene manager not available");
        return false;
    }
    
    SceneObject* object = currentProject_->getObject(objectId);
    if (!object) {
        notifyError("Object not found: " + objectId);
        return false;
    }
    
    // Record operation for undo
    DesignOperationRecord record(DesignOperation::MoveObject, objectId);
    record.beforeState = object->toJson();
    
    // Calculate new transform
    Transform3D newTransform = object->getTransform();
    newTransform.translation = newTransform.translation + translation;
    
    // Apply snapping
    newTransform.translation = applySnapping(newTransform.translation);
    
    // Validate new position
    if (!validateOperation(objectId, newTransform)) {
        notifyError("Invalid position for object");
        return false;
    }
    
    // Apply transform
    object->setTransform(newTransform);
    sceneManager_->moveObject(objectId, newTransform);
    
    record.afterState = object->toJson();
    recordOperation(record);
    
    return true;
}

bool DesignController::rotateObject(const std::string& objectId, const Vector3D& rotation) {
    if (!currentProject_ || !sceneManager_) {
        notifyError("No project loaded or scene manager not available");
        return false;
    }
    
    SceneObject* object = currentProject_->getObject(objectId);
    if (!object) {
        notifyError("Object not found: " + objectId);
        return false;
    }
    
    // Record operation for undo
    DesignOperationRecord record(DesignOperation::RotateObject, objectId);
    record.beforeState = object->toJson();
    
    // Calculate new transform
    Transform3D newTransform = object->getTransform();
    newTransform.rotation = newTransform.rotation + rotation;
    
    // Validate new transform
    if (!validateOperation(objectId, newTransform)) {
        notifyError("Invalid rotation for object");
        return false;
    }
    
    // Apply transform
    object->setTransform(newTransform);
    sceneManager_->moveObject(objectId, newTransform);
    
    record.afterState = object->toJson();
    recordOperation(record);
    
    return true;
}

bool DesignController::scaleObject(const std::string& objectId, const Vector3D& scale) {
    if (!currentProject_ || !sceneManager_) {
        notifyError("No project loaded or scene manager not available");
        return false;
    }
    
    SceneObject* object = currentProject_->getObject(objectId);
    if (!object) {
        notifyError("Object not found: " + objectId);
        return false;
    }
    
    // Record operation for undo
    DesignOperationRecord record(DesignOperation::ScaleObject, objectId);
    record.beforeState = object->toJson();
    
    // Calculate new transform
    Transform3D newTransform = object->getTransform();
    newTransform.scale = Vector3D(
        newTransform.scale.x * scale.x,
        newTransform.scale.y * scale.y,
        newTransform.scale.z * scale.z
    );
    
    // Validate new transform
    if (!validateOperation(objectId, newTransform)) {
        notifyError("Invalid scale for object");
        return false;
    }
    
    // Apply transform
    object->setTransform(newTransform);
    sceneManager_->moveObject(objectId, newTransform);
    
    record.afterState = object->toJson();
    recordOperation(record);
    
    return true;
}

bool DesignController::setObjectTransform(const std::string& objectId, const Transform3D& transform) {
    if (!currentProject_ || !sceneManager_) {
        notifyError("No project loaded or scene manager not available");
        return false;
    }
    
    SceneObject* object = currentProject_->getObject(objectId);
    if (!object) {
        notifyError("Object not found: " + objectId);
        return false;
    }
    
    // Record operation for undo
    DesignOperationRecord record(DesignOperation::ModifyObject, objectId);
    record.beforeState = object->toJson();
    
    // Apply snapping to position
    Transform3D snappedTransform = transform;
    snappedTransform.translation = applySnapping(transform.translation);
    
    // Validate new transform
    if (!validateOperation(objectId, snappedTransform)) {
        notifyError("Invalid transform for object");
        return false;
    }
    
    // Apply transform
    object->setTransform(snappedTransform);
    sceneManager_->moveObject(objectId, snappedTransform);
    
    record.afterState = object->toJson();
    recordOperation(record);
    
    return true;
}

std::string DesignController::duplicateObject(const std::string& objectId, const Vector3D& offset) {
    if (!currentProject_ || !sceneManager_) {
        notifyError("No project loaded or scene manager not available");
        return "";
    }
    
    const SceneObject* originalObject = currentProject_->getObject(objectId);
    if (!originalObject) {
        notifyError("Object not found: " + objectId);
        return "";
    }
    
    // Create duplicate with offset
    auto duplicate = std::make_unique<SceneObject>(*originalObject);
    Transform3D newTransform = duplicate->getTransform();
    newTransform.translation = newTransform.translation + offset;
    newTransform.translation = applySnapping(newTransform.translation);
    duplicate->setTransform(newTransform);
    
    // Validate placement
    if (!isValidPosition(duplicate->getCatalogItemId(), newTransform)) {
        notifyError("Invalid position for duplicated object");
        return "";
    }
    
    // Add to project and scene
    std::string newObjectId = currentProject_->addObject(std::make_unique<SceneObject>(*duplicate));
    sceneManager_->addObject(std::move(duplicate));
    
    // Record operation for undo
    DesignOperationRecord record(DesignOperation::AddObject, newObjectId);
    record.afterState = currentProject_->getObject(newObjectId)->toJson();
    recordOperation(record);
    
    return newObjectId;
}

void DesignController::selectObject(const std::string& objectId) {
    selectedObjects_.clear();
    selectedObjects_.push_back(objectId);
    updateSceneSelection();
    notifySelectionChanged();
}

void DesignController::addToSelection(const std::string& objectId) {
    if (!isSelected(objectId)) {
        selectedObjects_.push_back(objectId);
        updateSceneSelection();
        notifySelectionChanged();
    }
}

void DesignController::removeFromSelection(const std::string& objectId) {
    auto it = std::find(selectedObjects_.begin(), selectedObjects_.end(), objectId);
    if (it != selectedObjects_.end()) {
        selectedObjects_.erase(it);
        updateSceneSelection();
        notifySelectionChanged();
    }
}

void DesignController::selectObjects(const std::vector<std::string>& objectIds) {
    selectedObjects_ = objectIds;
    updateSceneSelection();
    notifySelectionChanged();
}

void DesignController::clearSelection() {
    selectedObjects_.clear();
    updateSceneSelection();
    notifySelectionChanged();
}

bool DesignController::isSelected(const std::string& objectId) const {
    return std::find(selectedObjects_.begin(), selectedObjects_.end(), objectId) != selectedObjects_.end();
}

void DesignController::selectObjectsInRegion(const BoundingBox& region) {
    if (!sceneManager_) {
        return;
    }
    
    auto objectsInRegion = sceneManager_->getObjectsInRegion(region);
    selectObjects(objectsInRegion);
}

std::string DesignController::addWall(const Point3D& start, const Point3D& end, double height, double thickness) {
    if (!currentProject_) {
        notifyError("No project loaded");
        return "";
    }
    
    // Apply snapping to wall endpoints
    Point3D snappedStart = applySnapping(start);
    Point3D snappedEnd = applySnapping(end);
    
    // Create wall
    Wall wall(SceneObject::generateId(), snappedStart, snappedEnd, height, thickness);
    
    if (!wall.isValid()) {
        notifyError("Invalid wall parameters");
        return "";
    }
    
    // Record operation for undo
    DesignOperationRecord record(DesignOperation::AddWall, wall.id);
    record.afterState = nlohmann::json{
        {"id", wall.id},
        {"start", {snappedStart.x, snappedStart.y, snappedStart.z}},
        {"end", {snappedEnd.x, snappedEnd.y, snappedEnd.z}},
        {"height", height},
        {"thickness", thickness}
    };
    
    currentProject_->addWall(wall);
    recordOperation(record);
    
    return wall.id;
}

bool DesignController::removeWall(const std::string& wallId) {
    if (!currentProject_) {
        notifyError("No project loaded");
        return false;
    }
    
    const Wall* wall = currentProject_->getWall(wallId);
    if (!wall) {
        notifyError("Wall not found: " + wallId);
        return false;
    }
    
    // Record operation for undo
    DesignOperationRecord record(DesignOperation::RemoveWall, wallId);
    record.beforeState = nlohmann::json{
        {"id", wall->id},
        {"start", {wall->start.x, wall->start.y, wall->start.z}},
        {"end", {wall->end.x, wall->end.y, wall->end.z}},
        {"height", wall->height},
        {"thickness", wall->thickness}
    };
    
    bool success = currentProject_->removeWall(wallId);
    if (success) {
        recordOperation(record);
    }
    
    return success;
}

bool DesignController::modifyWall(const std::string& wallId, const Point3D& start, const Point3D& end) {
    if (!currentProject_) {
        notifyError("No project loaded");
        return false;
    }
    
    Wall* wall = currentProject_->getWall(wallId);
    if (!wall) {
        notifyError("Wall not found: " + wallId);
        return false;
    }
    
    // Record operation for undo
    DesignOperationRecord record(DesignOperation::ModifyObject, wallId);
    record.beforeState = nlohmann::json{
        {"id", wall->id},
        {"start", {wall->start.x, wall->start.y, wall->start.z}},
        {"end", {wall->end.x, wall->end.y, wall->end.z}},
        {"height", wall->height},
        {"thickness", wall->thickness}
    };
    
    // Apply snapping and update wall
    wall->start = applySnapping(start);
    wall->end = applySnapping(end);
    
    if (!wall->isValid()) {
        notifyError("Invalid wall parameters");
        return false;
    }
    
    record.afterState = nlohmann::json{
        {"id", wall->id},
        {"start", {wall->start.x, wall->start.y, wall->start.z}},
        {"end", {wall->end.x, wall->end.y, wall->end.z}},
        {"height", wall->height},
        {"thickness", wall->thickness}
    };
    
    recordOperation(record);
    return true;
}

std::string DesignController::addOpening(const std::string& wallId, const std::string& type, 
                                        double position, double width, double height, double sillHeight) {
    if (!currentProject_) {
        notifyError("No project loaded");
        return "";
    }
    
    const Wall* wall = currentProject_->getWall(wallId);
    if (!wall) {
        notifyError("Wall not found: " + wallId);
        return "";
    }
    
    // Create opening
    Opening opening(SceneObject::generateId(), wallId, type, position, width, height, sillHeight);
    
    if (!opening.isValid()) {
        notifyError("Invalid opening parameters");
        return "";
    }
    
    // Record operation for undo
    DesignOperationRecord record(DesignOperation::AddOpening, opening.id);
    record.afterState = nlohmann::json{
        {"id", opening.id},
        {"wallId", wallId},
        {"type", type},
        {"position", position},
        {"width", width},
        {"height", height},
        {"sillHeight", sillHeight}
    };
    
    currentProject_->addOpening(opening);
    recordOperation(record);
    
    return opening.id;
}

bool DesignController::removeOpening(const std::string& openingId) {
    if (!currentProject_) {
        notifyError("No project loaded");
        return false;
    }
    
    const Opening* opening = currentProject_->getOpening(openingId);
    if (!opening) {
        notifyError("Opening not found: " + openingId);
        return false;
    }
    
    // Record operation for undo
    DesignOperationRecord record(DesignOperation::RemoveOpening, openingId);
    record.beforeState = nlohmann::json{
        {"id", opening->id},
        {"wallId", opening->wallId},
        {"type", opening->type},
        {"position", opening->position},
        {"width", opening->width},
        {"height", opening->height},
        {"sillHeight", opening->sillHeight}
    };
    
    bool success = currentProject_->removeOpening(openingId);
    if (success) {
        recordOperation(record);
    }
    
    return success;
}

void DesignController::setGridSettings(const GridSettings& settings) {
    gridSettings_ = settings;
}

Point3D DesignController::snapToGrid(const Point3D& point) const {
    if (!gridSettings_.enabled || !gridSettings_.snapToGrid) {
        return point;
    }
    
    double spacing = gridSettings_.spacing;
    return Point3D(
        std::round(point.x / spacing) * spacing,
        std::round(point.y / spacing) * spacing,
        std::round(point.z / spacing) * spacing
    );
}

Point3D DesignController::snapToObjects(const Point3D& point) const {
    if (!gridSettings_.snapToObjects || !sceneManager_) {
        return point;
    }
    
    double snapDistance = gridSettings_.snapDistance;
    Point3D closestPoint = point;
    double minDistance = snapDistance;
    
    // Check all objects for snap points
    sceneManager_->forEachObject([&](const ObjectId& id, const SceneObject* object) {
        if (object) {
            // Get object bounds and check corners/edges for snapping
            Transform3D transform = object->getTransform();
            Point3D objectPos = transform.translation;
            
            double distance = point.distanceTo(objectPos);
            if (distance < minDistance) {
                minDistance = distance;
                closestPoint = objectPos;
            }
        }
    });
    
    return closestPoint;
}

Point3D DesignController::applySnapping(const Point3D& point) const {
    Point3D snappedPoint = point;
    
    // Apply object snapping first
    snappedPoint = snapToObjects(snappedPoint);
    
    // Then apply grid snapping
    snappedPoint = snapToGrid(snappedPoint);
    
    return snappedPoint;
}

std::vector<ValidationError> DesignController::validateDesign() const {
    if (!currentProject_ || !validationService_) {
        return {};
    }
    
    return validationService_->validateProject(*currentProject_);
}

std::vector<ValidationError> DesignController::validateObjectPlacement(const std::string& objectId, 
                                                                      const Transform3D& transform) const {
    if (!currentProject_ || !validationService_) {
        return {};
    }
    
    const SceneObject* object = currentProject_->getObject(objectId);
    if (!object) {
        return {ValidationError(ValidationSeverity::Error, "Object not found", objectId)};
    }
    
    // Create temporary object with new transform for validation
    SceneObject tempObject(*object);
    tempObject.setTransform(transform);
    
    return validationService_->validateObject(tempObject, *sceneManager_);
}

bool DesignController::isValidPosition(const std::string& catalogItemId, const Transform3D& transform) const {
    if (!validationService_ || !sceneManager_) {
        return true; // Assume valid if no validation service
    }
    
    // Create temporary object for validation
    SceneObject tempObject(catalogItemId);
    tempObject.setTransform(transform);
    
    auto errors = validationService_->validatePlacement(tempObject, transform, *sceneManager_);
    
    // Check if there are any critical errors
    for (const auto& error : errors) {
        if (error.severity == ValidationSeverity::Error || error.severity == ValidationSeverity::Critical) {
            return false;
        }
    }
    
    return true;
}

bool DesignController::undo() {
    if (!canUndo()) {
        return false;
    }
    
    const auto& record = operationHistory_[currentHistoryIndex_ - 1];
    
    // Apply reverse operation based on type
    bool success = false;
    switch (record.operation) {
        case DesignOperation::AddObject:
            success = removeObject(record.objectId);
            break;
        case DesignOperation::RemoveObject:
            // Recreate object from beforeState
            if (!record.beforeState.empty()) {
                auto object = std::make_unique<SceneObject>();
                object->fromJson(record.beforeState);
                currentProject_->addObject(std::move(object));
                success = true;
            }
            break;
        case DesignOperation::MoveObject:
        case DesignOperation::RotateObject:
        case DesignOperation::ScaleObject:
        case DesignOperation::ModifyObject:
            // Restore object from beforeState
            if (!record.beforeState.empty()) {
                SceneObject* object = currentProject_->getObject(record.objectId);
                if (object) {
                    object->fromJson(record.beforeState);
                    sceneManager_->moveObject(record.objectId, object->getTransform());
                    success = true;
                }
            }
            break;
        // Add cases for walls and openings...
    }
    
    if (success) {
        currentHistoryIndex_--;
    }
    
    return success;
}

bool DesignController::redo() {
    if (!canRedo()) {
        return false;
    }
    
    const auto& record = operationHistory_[currentHistoryIndex_];
    
    // Apply operation based on type
    bool success = false;
    switch (record.operation) {
        case DesignOperation::AddObject:
            // Recreate object from afterState
            if (!record.afterState.empty()) {
                auto object = std::make_unique<SceneObject>();
                object->fromJson(record.afterState);
                currentProject_->addObject(std::move(object));
                success = true;
            }
            break;
        case DesignOperation::RemoveObject:
            success = removeObject(record.objectId);
            break;
        case DesignOperation::MoveObject:
        case DesignOperation::RotateObject:
        case DesignOperation::ScaleObject:
        case DesignOperation::ModifyObject:
            // Restore object from afterState
            if (!record.afterState.empty()) {
                SceneObject* object = currentProject_->getObject(record.objectId);
                if (object) {
                    object->fromJson(record.afterState);
                    sceneManager_->moveObject(record.objectId, object->getTransform());
                    success = true;
                }
            }
            break;
        // Add cases for walls and openings...
    }
    
    if (success) {
        currentHistoryIndex_++;
    }
    
    return success;
}

bool DesignController::canUndo() const {
    return currentHistoryIndex_ > 0;
}

bool DesignController::canRedo() const {
    return currentHistoryIndex_ < operationHistory_.size();
}

void DesignController::clearHistory() {
    operationHistory_.clear();
    currentHistoryIndex_ = 0;
}

double DesignController::measureDistance(const Point3D& point1, const Point3D& point2) const {
    return point1.distanceTo(point2);
}

double DesignController::calculateSelectedArea() const {
    // Implementation would calculate total surface area of selected objects
    // This is a placeholder implementation
    return 0.0;
}

double DesignController::calculateSelectedVolume() const {
    // Implementation would calculate total volume of selected objects
    // This is a placeholder implementation
    return 0.0;
}

BoundingBox DesignController::getSelectionBounds() const {
    if (selectedObjects_.empty() || !currentProject_) {
        return BoundingBox();
    }
    
    BoundingBox bounds;
    bool first = true;
    
    for (const auto& objectId : selectedObjects_) {
        const SceneObject* object = currentProject_->getObject(objectId);
        if (object) {
            // Calculate object bounds (placeholder implementation)
            Transform3D transform = object->getTransform();
            Point3D pos = transform.translation;
            
            if (first) {
                bounds = BoundingBox(pos, pos);
                first = false;
            } else {
                // Expand bounds to include this object
                if (pos.x < bounds.min.x) bounds.min.x = pos.x;
                if (pos.y < bounds.min.y) bounds.min.y = pos.y;
                if (pos.z < bounds.min.z) bounds.min.z = pos.z;
                if (pos.x > bounds.max.x) bounds.max.x = pos.x;
                if (pos.y > bounds.max.y) bounds.max.y = pos.y;
                if (pos.z > bounds.max.z) bounds.max.z = pos.z;
            }
        }
    }
    
    return bounds;
}

bool DesignController::alignObjects(AlignmentType alignment) {
    if (selectedObjects_.size() < 2 || !currentProject_) {
        return false;
    }
    
    BoundingBox bounds = getSelectionBounds();
    
    for (const auto& objectId : selectedObjects_) {
        SceneObject* object = currentProject_->getObject(objectId);
        if (object) {
            Transform3D transform = object->getTransform();
            
            switch (alignment) {
                case AlignmentType::Left:
                    transform.translation.x = bounds.min.x;
                    break;
                case AlignmentType::Right:
                    transform.translation.x = bounds.max.x;
                    break;
                case AlignmentType::Center:
                    transform.translation.x = (bounds.min.x + bounds.max.x) / 2.0;
                    break;
                case AlignmentType::Top:
                    transform.translation.y = bounds.max.y;
                    break;
                case AlignmentType::Bottom:
                    transform.translation.y = bounds.min.y;
                    break;
                case AlignmentType::Middle:
                    transform.translation.y = (bounds.min.y + bounds.max.y) / 2.0;
                    break;
                case AlignmentType::Front:
                    transform.translation.z = bounds.min.z;
                    break;
                case AlignmentType::Back:
                    transform.translation.z = bounds.max.z;
                    break;
            }
            
            object->setTransform(transform);
            sceneManager_->moveObject(objectId, transform);
        }
    }
    
    return true;
}

bool DesignController::distributeObjects(DistributionType distribution) {
    if (selectedObjects_.size() < 3 || !currentProject_) {
        return false;
    }
    
    // Sort objects by position along the distribution axis
    std::vector<std::pair<std::string, double>> sortedObjects;
    
    for (const auto& objectId : selectedObjects_) {
        const SceneObject* object = currentProject_->getObject(objectId);
        if (object) {
            Transform3D transform = object->getTransform();
            double position = 0.0;
            
            switch (distribution) {
                case DistributionType::Horizontal:
                    position = transform.translation.x;
                    break;
                case DistributionType::Vertical:
                    position = transform.translation.y;
                    break;
                case DistributionType::Depth:
                    position = transform.translation.z;
                    break;
            }
            
            sortedObjects.push_back({objectId, position});
        }
    }
    
    std::sort(sortedObjects.begin(), sortedObjects.end(), 
              [](const auto& a, const auto& b) { return a.second < b.second; });
    
    // Calculate spacing
    double totalDistance = sortedObjects.back().second - sortedObjects.front().second;
    double spacing = totalDistance / (sortedObjects.size() - 1);
    
    // Distribute objects
    for (size_t i = 1; i < sortedObjects.size() - 1; ++i) {
        SceneObject* object = currentProject_->getObject(sortedObjects[i].first);
        if (object) {
            Transform3D transform = object->getTransform();
            double newPosition = sortedObjects.front().second + i * spacing;
            
            switch (distribution) {
                case DistributionType::Horizontal:
                    transform.translation.x = newPosition;
                    break;
                case DistributionType::Vertical:
                    transform.translation.y = newPosition;
                    break;
                case DistributionType::Depth:
                    transform.translation.z = newPosition;
                    break;
            }
            
            object->setTransform(transform);
            sceneManager_->moveObject(sortedObjects[i].first, transform);
        }
    }
    
    return true;
}

void DesignController::recordOperation(const DesignOperationRecord& record) {
    // Remove any operations after current index (for redo)
    if (currentHistoryIndex_ < operationHistory_.size()) {
        operationHistory_.erase(operationHistory_.begin() + currentHistoryIndex_, operationHistory_.end());
    }
    
    // Add new operation
    operationHistory_.push_back(record);
    currentHistoryIndex_ = operationHistory_.size();
    
    // Cleanup if history is too large
    cleanupHistory();
}

void DesignController::notifyObjectAdded(const std::string& objectId) {
    if (objectAddedCallback_) {
        objectAddedCallback_(objectId);
    }
}

void DesignController::notifyObjectRemoved(const std::string& objectId) {
    if (objectRemovedCallback_) {
        objectRemovedCallback_(objectId);
    }
}

void DesignController::notifyObjectModified(const std::string& objectId) {
    if (objectModifiedCallback_) {
        objectModifiedCallback_(objectId);
    }
}

void DesignController::notifySelectionChanged() {
    if (selectionChangedCallback_) {
        selectionChangedCallback_(selectedObjects_);
    }
}

void DesignController::notifyValidation(const std::vector<ValidationError>& errors) {
    if (validationCallback_) {
        validationCallback_(errors);
    }
}

void DesignController::notifyError(const std::string& error) {
    if (errorCallback_) {
        errorCallback_(error);
    }
}

void DesignController::updateSceneSelection() {
    if (sceneManager_) {
        sceneManager_->setSelection(selectedObjects_);
    }
}

bool DesignController::validateOperation(const std::string& objectId, const Transform3D& transform) const {
    auto errors = validateObjectPlacement(objectId, transform);
    
    // Check for critical errors
    for (const auto& error : errors) {
        if (error.severity == ValidationSeverity::Error || error.severity == ValidationSeverity::Critical) {
            return false;
        }
    }
    
    return true;
}

void DesignController::cleanupHistory() {
    if (operationHistory_.size() > maxHistorySize_) {
        size_t removeCount = operationHistory_.size() - maxHistorySize_;
        operationHistory_.erase(operationHistory_.begin(), operationHistory_.begin() + removeCount);
        currentHistoryIndex_ -= removeCount;
    }
}

Point3D DesignController::getCurrentObjectPosition(const std::string& objectId) const {
    if (!currentProject_) {
        return Point3D();
    }
    
    const SceneObject* object = currentProject_->getObject(objectId);
    if (object) {
        return object->getTransform().translation;
    }
    
    return Point3D();
}

} // namespace Controllers
} // namespace KitchenCAD