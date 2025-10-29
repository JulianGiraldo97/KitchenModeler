#pragma once

#include <memory>
#include <vector>
#include <string>
#include <functional>
#include "../interfaces/ISceneManager.h"
#include "../interfaces/IGeometryEngine.h"
#include "../interfaces/IValidationService.h"
#include "../interfaces/IProjectRepository.h"
#include "../models/Project.h"
#include "../models/CatalogItem.h"
#include "../geometry/Point3D.h"
#include "../geometry/Vector3D.h"
#include "../geometry/Transform3D.h"

namespace KitchenCAD {
namespace Controllers {

using namespace Models;
using namespace Geometry;

/**
 * @brief Design operation types for undo/redo system
 */
enum class DesignOperation {
    AddObject,
    RemoveObject,
    MoveObject,
    RotateObject,
    ScaleObject,
    ModifyObject,
    AddWall,
    RemoveWall,
    AddOpening,
    RemoveOpening
};

/**
 * @brief Design operation record for undo/redo
 */
struct DesignOperationRecord {
    DesignOperation operation;
    std::string objectId;
    nlohmann::json beforeState;
    nlohmann::json afterState;
    std::chrono::system_clock::time_point timestamp;
    
    DesignOperationRecord(DesignOperation op, const std::string& id)
        : operation(op), objectId(id), timestamp(std::chrono::system_clock::now()) {}
};

/**
 * @brief Grid and snap settings
 */
struct GridSettings {
    bool enabled = true;
    double spacing = 0.05; // 5cm grid
    bool snapToGrid = true;
    bool snapToObjects = true;
    double snapDistance = 0.02; // 2cm snap distance
    bool showGrid = true;
    
    GridSettings() = default;
};

/**
 * @brief Design controller for managing design operations
 * 
 * This controller handles all design-related operations including object placement,
 * manipulation, validation, and design tools. Implements requirements 1.1, 4.1, 5.1.
 */
class DesignController {
private:
    std::unique_ptr<ISceneManager> sceneManager_;
    std::unique_ptr<IGeometryEngine> geometryEngine_;
    std::unique_ptr<IValidationService> validationService_;
    Project* currentProject_;
    
    // Operation history for undo/redo
    std::vector<DesignOperationRecord> operationHistory_;
    size_t currentHistoryIndex_;
    size_t maxHistorySize_;
    
    // Grid and snap settings
    GridSettings gridSettings_;
    
    // Selection state
    std::vector<std::string> selectedObjects_;
    
    // Callbacks
    std::function<void(const std::string&)> objectAddedCallback_;
    std::function<void(const std::string&)> objectRemovedCallback_;
    std::function<void(const std::string&)> objectModifiedCallback_;
    std::function<void(const std::vector<std::string>&)> selectionChangedCallback_;
    std::function<void(const std::vector<ValidationError>&)> validationCallback_;
    std::function<void(const std::string&)> errorCallback_;

public:
    /**
     * @brief Constructor
     */
    DesignController(std::unique_ptr<ISceneManager> sceneManager,
                    std::unique_ptr<IGeometryEngine> geometryEngine,
                    std::unique_ptr<IValidationService> validationService);
    
    /**
     * @brief Destructor
     */
    ~DesignController() = default;
    
    // Project management
    
    /**
     * @brief Set current project
     */
    void setCurrentProject(Project* project);
    
    /**
     * @brief Get current project
     */
    Project* getCurrentProject() const { return currentProject_; }
    
    /**
     * @brief Check if project is loaded
     */
    bool hasProject() const { return currentProject_ != nullptr; }
    
    // Object placement and manipulation
    
    /**
     * @brief Add object from catalog to scene
     */
    std::string addObjectFromCatalog(const std::string& catalogItemId, const Point3D& position);
    
    /**
     * @brief Add object with specific transform
     */
    std::string addObjectFromCatalog(const std::string& catalogItemId, const Transform3D& transform);
    
    /**
     * @brief Remove object from scene
     */
    bool removeObject(const std::string& objectId);
    
    /**
     * @brief Move object to new position
     */
    bool moveObject(const std::string& objectId, const Point3D& newPosition);
    
    /**
     * @brief Move object by offset
     */
    bool translateObject(const std::string& objectId, const Vector3D& translation);
    
    /**
     * @brief Rotate object
     */
    bool rotateObject(const std::string& objectId, const Vector3D& rotation);
    
    /**
     * @brief Scale object
     */
    bool scaleObject(const std::string& objectId, const Vector3D& scale);
    
    /**
     * @brief Set object transform
     */
    bool setObjectTransform(const std::string& objectId, const Transform3D& transform);
    
    /**
     * @brief Duplicate object
     */
    std::string duplicateObject(const std::string& objectId, const Vector3D& offset = Vector3D(0.1, 0, 0));
    
    // Selection management
    
    /**
     * @brief Select single object
     */
    void selectObject(const std::string& objectId);
    
    /**
     * @brief Add object to selection
     */
    void addToSelection(const std::string& objectId);
    
    /**
     * @brief Remove object from selection
     */
    void removeFromSelection(const std::string& objectId);
    
    /**
     * @brief Select multiple objects
     */
    void selectObjects(const std::vector<std::string>& objectIds);
    
    /**
     * @brief Clear selection
     */
    void clearSelection();
    
    /**
     * @brief Get current selection
     */
    const std::vector<std::string>& getSelection() const { return selectedObjects_; }
    
    /**
     * @brief Check if object is selected
     */
    bool isSelected(const std::string& objectId) const;
    
    /**
     * @brief Select objects in region
     */
    void selectObjectsInRegion(const BoundingBox& region);
    
    // Structural elements (walls, openings)
    
    /**
     * @brief Add wall to project
     */
    std::string addWall(const Point3D& start, const Point3D& end, double height, double thickness = 0.1);
    
    /**
     * @brief Remove wall from project
     */
    bool removeWall(const std::string& wallId);
    
    /**
     * @brief Modify wall endpoints
     */
    bool modifyWall(const std::string& wallId, const Point3D& start, const Point3D& end);
    
    /**
     * @brief Add opening (door/window) to wall
     */
    std::string addOpening(const std::string& wallId, const std::string& type, 
                          double position, double width, double height, double sillHeight = 0.0);
    
    /**
     * @brief Remove opening from wall
     */
    bool removeOpening(const std::string& openingId);
    
    // Grid and snap functionality
    
    /**
     * @brief Set grid settings
     */
    void setGridSettings(const GridSettings& settings);
    
    /**
     * @brief Get grid settings
     */
    const GridSettings& getGridSettings() const { return gridSettings_; }
    
    /**
     * @brief Snap point to grid
     */
    Point3D snapToGrid(const Point3D& point) const;
    
    /**
     * @brief Snap point to nearby objects
     */
    Point3D snapToObjects(const Point3D& point) const;
    
    /**
     * @brief Apply all snapping rules
     */
    Point3D applySnapping(const Point3D& point) const;
    
    // Validation
    
    /**
     * @brief Validate current design
     */
    std::vector<ValidationError> validateDesign() const;
    
    /**
     * @brief Validate object placement
     */
    std::vector<ValidationError> validateObjectPlacement(const std::string& objectId, 
                                                        const Transform3D& transform) const;
    
    /**
     * @brief Check if position is valid for object
     */
    bool isValidPosition(const std::string& catalogItemId, const Transform3D& transform) const;
    
    // Undo/Redo system
    
    /**
     * @brief Undo last operation
     */
    bool undo();
    
    /**
     * @brief Redo last undone operation
     */
    bool redo();
    
    /**
     * @brief Check if undo is available
     */
    bool canUndo() const;
    
    /**
     * @brief Check if redo is available
     */
    bool canRedo() const;
    
    /**
     * @brief Clear operation history
     */
    void clearHistory();
    
    /**
     * @brief Set maximum history size
     */
    void setMaxHistorySize(size_t maxSize) { maxHistorySize_ = maxSize; }
    
    // Measurement and analysis
    
    /**
     * @brief Measure distance between two points
     */
    double measureDistance(const Point3D& point1, const Point3D& point2) const;
    
    /**
     * @brief Calculate area of selected objects
     */
    double calculateSelectedArea() const;
    
    /**
     * @brief Calculate volume of selected objects
     */
    double calculateSelectedVolume() const;
    
    /**
     * @brief Get bounding box of selection
     */
    BoundingBox getSelectionBounds() const;
    
    // Alignment and distribution tools
    
    /**
     * @brief Align selected objects
     */
    enum class AlignmentType { Left, Right, Center, Top, Bottom, Middle, Front, Back };
    bool alignObjects(AlignmentType alignment);
    
    /**
     * @brief Distribute selected objects evenly
     */
    enum class DistributionType { Horizontal, Vertical, Depth };
    bool distributeObjects(DistributionType distribution);
    
    // Callbacks
    
    /**
     * @brief Set callback for object added events
     */
    void setObjectAddedCallback(std::function<void(const std::string&)> callback) {
        objectAddedCallback_ = callback;
    }
    
    /**
     * @brief Set callback for object removed events
     */
    void setObjectRemovedCallback(std::function<void(const std::string&)> callback) {
        objectRemovedCallback_ = callback;
    }
    
    /**
     * @brief Set callback for object modified events
     */
    void setObjectModifiedCallback(std::function<void(const std::string&)> callback) {
        objectModifiedCallback_ = callback;
    }
    
    /**
     * @brief Set callback for selection changed events
     */
    void setSelectionChangedCallback(std::function<void(const std::vector<std::string>&)> callback) {
        selectionChangedCallback_ = callback;
    }
    
    /**
     * @brief Set callback for validation events
     */
    void setValidationCallback(std::function<void(const std::vector<ValidationError>&)> callback) {
        validationCallback_ = callback;
    }
    
    /**
     * @brief Set callback for errors
     */
    void setErrorCallback(std::function<void(const std::string&)> callback) {
        errorCallback_ = callback;
    }

private:
    /**
     * @brief Record operation for undo/redo
     */
    void recordOperation(const DesignOperationRecord& record);
    
    /**
     * @brief Notify callbacks
     */
    void notifyObjectAdded(const std::string& objectId);
    void notifyObjectRemoved(const std::string& objectId);
    void notifyObjectModified(const std::string& objectId);
    void notifySelectionChanged();
    void notifyValidation(const std::vector<ValidationError>& errors);
    void notifyError(const std::string& error);
    
    /**
     * @brief Update selection in scene manager
     */
    void updateSceneSelection();
    
    /**
     * @brief Validate operation before execution
     */
    bool validateOperation(const std::string& objectId, const Transform3D& transform) const;
    
    /**
     * @brief Cleanup history if it exceeds max size
     */
    void cleanupHistory();
};

} // namespace Controllers
} // namespace KitchenCAD