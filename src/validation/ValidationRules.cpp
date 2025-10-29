#include "ValidationRules.h"
#include "../models/Project.h"
#include "../scene/SceneManager.h"
#include <algorithm>
#include <cmath>

namespace KitchenCAD {
namespace Validation {

// CollisionValidationRule Implementation

CollisionValidationRule::CollisionValidationRule()
    : BaseValidationRule("collision.overlap", "Collision Detection", 
                        "Detects overlapping objects and spatial conflicts", 
                        ValidationSeverity::Error) {}

std::vector<ValidationError> CollisionValidationRule::validate(const SceneObject& object,
                                                              const ValidationContext& context) {
    std::vector<ValidationError> errors;
    
    if (!context.sceneManager) {
        return errors; // Cannot validate without scene context
    }
    
    // Get object's bounding box
    Geometry::BoundingBox objectBounds = calculateTransformedBounds(object);
    
    // Check collision with all other objects
    auto intersectingObjects = context.sceneManager->findIntersectingObjects(object.getId());
    
    for (const auto& otherId : intersectingObjects) {
        const SceneObject* otherObject = context.sceneManager->getObject(otherId);
        if (otherObject && otherObject->getId() != object.getId()) {
            Geometry::BoundingBox otherBounds = calculateTransformedBounds(*otherObject);
            
            if (checkBoundingBoxCollision(objectBounds, otherBounds)) {
                errors.emplace_back(ValidationSeverity::Error,
                                   "Object collision detected with " + otherId,
                                   object.getId(),
                                   object.getTransform().translation,
                                   "Move objects to avoid overlap",
                                   getRuleId());
            }
        }
    }
    
    return errors;
}

bool CollisionValidationRule::appliesTo(const SceneObject& object) const {
    // Applies to all objects
    return true;
}

bool CollisionValidationRule::checkBoundingBoxCollision(const Geometry::BoundingBox& a, 
                                                       const Geometry::BoundingBox& b) const {
    return a.intersects(b);
}

Geometry::BoundingBox CollisionValidationRule::calculateTransformedBounds(const SceneObject& object) const {
    // For now, create a simple bounding box based on transform
    // In a real implementation, this would use the actual geometry
    const auto& transform = object.getTransform();
    const auto& pos = transform.translation;
    const auto& scale = transform.scale;
    
    // Assume a default size of 0.6m x 0.6m x 0.85m (typical base cabinet)
    double width = 0.6 * scale.x;
    double depth = 0.6 * scale.y;
    double height = 0.85 * scale.z;
    
    return Geometry::BoundingBox(
        Geometry::Point3D(pos.x - width/2, pos.y - depth/2, pos.z),
        Geometry::Point3D(pos.x + width/2, pos.y + depth/2, pos.z + height)
    );
}

// DimensionValidationRule Implementation

DimensionValidationRule::DimensionValidationRule(double minDim, double maxDim)
    : BaseValidationRule("dimension.limits", "Dimension Validation",
                        "Validates object dimensions are within acceptable ranges",
                        ValidationSeverity::Warning),
      minDimension_(minDim), maxDimension_(maxDim) {}

std::vector<ValidationError> DimensionValidationRule::validate(const SceneObject& object,
                                                              const ValidationContext& context) {
    std::vector<ValidationError> errors;
    
    Geometry::Vector3D dimensions = getObjectDimensions(object);
    
    // Check minimum dimensions
    if (dimensions.x < minDimension_ || dimensions.y < minDimension_ || dimensions.z < minDimension_) {
        errors.emplace_back(ValidationSeverity::Warning,
                           "Object dimensions too small (min: " + std::to_string(minDimension_) + "m)",
                           object.getId(),
                           object.getTransform().translation,
                           "Increase object size or check catalog specifications",
                           getRuleId());
    }
    
    // Check maximum dimensions
    if (dimensions.x > maxDimension_ || dimensions.y > maxDimension_ || dimensions.z > maxDimension_) {
        errors.emplace_back(ValidationSeverity::Warning,
                           "Object dimensions too large (max: " + std::to_string(maxDimension_) + "m)",
                           object.getId(),
                           object.getTransform().translation,
                           "Reduce object size or check if it fits the space",
                           getRuleId());
    }
    
    // Check if object fits within project bounds
    if (context.project) {
        const auto& roomDims = context.project->getDimensions();
        const auto& pos = object.getTransform().translation;
        
        if (pos.x + dimensions.x/2 > roomDims.width ||
            pos.y + dimensions.y/2 > roomDims.depth ||
            pos.z + dimensions.z > roomDims.height) {
            errors.emplace_back(ValidationSeverity::Error,
                               "Object exceeds room boundaries",
                               object.getId(),
                               pos,
                               "Resize object or move within room limits",
                               getRuleId());
        }
    }
    
    return errors;
}

bool DimensionValidationRule::appliesTo(const SceneObject& object) const {
    // Applies to all objects
    return true;
}

Geometry::Vector3D DimensionValidationRule::getObjectDimensions(const SceneObject& object) const {
    // Default dimensions based on scale (would be replaced with actual geometry data)
    const auto& scale = object.getTransform().scale;
    return Geometry::Vector3D(0.6 * scale.x, 0.6 * scale.y, 0.85 * scale.z);
}

// HeightValidationRule Implementation

HeightValidationRule::HeightValidationRule()
    : BaseValidationRule("height.limits", "Height Validation",
                        "Validates object heights for kitchen standards",
                        ValidationSeverity::Warning),
      minHeight_(0.1), maxHeight_(2.5),
      standardCounterHeight_(0.85), standardWallCabinetHeight_(0.72) {}

std::vector<ValidationError> HeightValidationRule::validate(const SceneObject& object,
                                                           const ValidationContext& context) {
    std::vector<ValidationError> errors;
    
    std::string objectType = getObjectType(object);
    double expectedHeight = getExpectedHeight(object);
    double actualHeight = getObjectDimensions(object).z;
    
    // Check if height is within acceptable range for object type
    double tolerance = 0.05; // 5cm tolerance
    
    if (std::abs(actualHeight - expectedHeight) > tolerance) {
        std::string message = "Non-standard height for " + objectType + 
                             " (expected: " + std::to_string(expectedHeight) + 
                             "m, actual: " + std::to_string(actualHeight) + "m)";
        
        errors.emplace_back(ValidationSeverity::Warning,
                           message,
                           object.getId(),
                           object.getTransform().translation,
                           "Adjust height to match kitchen standards",
                           getRuleId());
    }
    
    // Check absolute height limits
    if (actualHeight < minHeight_) {
        errors.emplace_back(ValidationSeverity::Error,
                           "Object height too low (min: " + std::to_string(minHeight_) + "m)",
                           object.getId(),
                           object.getTransform().translation,
                           "Increase object height",
                           getRuleId());
    }
    
    if (actualHeight > maxHeight_) {
        errors.emplace_back(ValidationSeverity::Error,
                           "Object height too high (max: " + std::to_string(maxHeight_) + "m)",
                           object.getId(),
                           object.getTransform().translation,
                           "Reduce object height or check ceiling clearance",
                           getRuleId());
    }
    
    return errors;
}

bool HeightValidationRule::appliesTo(const SceneObject& object) const {
    // Applies to kitchen modules and furniture
    std::string catalogId = object.getCatalogItemId();
    return catalogId.find("cabinet") != std::string::npos ||
           catalogId.find("counter") != std::string::npos ||
           catalogId.find("appliance") != std::string::npos;
}

double HeightValidationRule::getExpectedHeight(const SceneObject& object) const {
    std::string type = getObjectType(object);
    
    if (type.find("base") != std::string::npos || type.find("counter") != std::string::npos) {
        return standardCounterHeight_;
    } else if (type.find("wall") != std::string::npos || type.find("upper") != std::string::npos) {
        return standardWallCabinetHeight_;
    } else if (type.find("tall") != std::string::npos || type.find("pantry") != std::string::npos) {
        return 2.1; // Standard tall cabinet height
    }
    
    return standardCounterHeight_; // Default
}

std::string HeightValidationRule::getObjectType(const SceneObject& object) const {
    return object.getCatalogItemId(); // Simplified - would parse actual type
}

Geometry::Vector3D HeightValidationRule::getObjectDimensions(const SceneObject& object) const {
    // Same as DimensionValidationRule - would be shared utility
    const auto& scale = object.getTransform().scale;
    return Geometry::Vector3D(0.6 * scale.x, 0.6 * scale.y, 0.85 * scale.z);
}

// ClearanceValidationRule Implementation

ClearanceValidationRule::ClearanceValidationRule()
    : BaseValidationRule("clearance.requirements", "Clearance Validation",
                        "Validates minimum clearances for accessibility and functionality",
                        ValidationSeverity::Warning),
      minDoorClearance_(0.9), minWalkwayClearance_(1.0), minWorkspaceClearance_(0.6) {}

std::vector<ValidationError> ClearanceValidationRule::validate(const SceneObject& object,
                                                              const ValidationContext& context) {
    std::vector<ValidationError> errors;
    
    // Check door clearances
    auto doorErrors = checkDoorClearance(object, context);
    errors.insert(errors.end(), doorErrors.begin(), doorErrors.end());
    
    // Check walkway clearances
    auto walkwayErrors = checkWalkwayClearance(object, context);
    errors.insert(errors.end(), walkwayErrors.begin(), walkwayErrors.end());
    
    // Check workspace clearances
    auto workspaceErrors = checkWorkspaceClearance(object, context);
    errors.insert(errors.end(), workspaceErrors.begin(), workspaceErrors.end());
    
    return errors;
}

bool ClearanceValidationRule::appliesTo(const SceneObject& object) const {
    // Applies to objects that might block clearances
    return true;
}

std::vector<ValidationError> ClearanceValidationRule::checkDoorClearance(const SceneObject& object,
                                                                        const ValidationContext& context) const {
    std::vector<ValidationError> errors;
    
    if (!hasDoors(object)) {
        return errors;
    }
    
    Geometry::BoundingBox clearanceArea = getDoorClearanceArea(object);
    
    if (context.sceneManager) {
        auto intersectingObjects = context.sceneManager->getObjectsInRegion(clearanceArea);
        
        for (const auto& otherId : intersectingObjects) {
            if (otherId != object.getId()) {
                errors.emplace_back(ValidationSeverity::Warning,
                                   "Insufficient door clearance - blocked by " + otherId,
                                   object.getId(),
                                   object.getTransform().translation,
                                   "Ensure " + std::to_string(minDoorClearance_) + "m clearance for door opening",
                                   getRuleId());
            }
        }
    }
    
    return errors;
}

std::vector<ValidationError> ClearanceValidationRule::checkWalkwayClearance(const SceneObject& object,
                                                                           const ValidationContext& context) const {
    std::vector<ValidationError> errors;
    
    // Check if object blocks main walkways
    const auto& pos = object.getTransform().translation;
    
    // Simple check - in a real implementation, this would be more sophisticated
    if (context.project) {
        const auto& roomDims = context.project->getDimensions();
        
        // Check if object is in the center of the room (main walkway)
        double centerX = roomDims.width / 2.0;
        double centerY = roomDims.depth / 2.0;
        
        if (std::abs(pos.x - centerX) < minWalkwayClearance_/2 && 
            std::abs(pos.y - centerY) < minWalkwayClearance_/2) {
            errors.emplace_back(ValidationSeverity::Warning,
                               "Object may block main walkway",
                               object.getId(),
                               pos,
                               "Maintain " + std::to_string(minWalkwayClearance_) + "m walkway clearance",
                               getRuleId());
        }
    }
    
    return errors;
}

std::vector<ValidationError> ClearanceValidationRule::checkWorkspaceClearance(const SceneObject& object,
                                                                             const ValidationContext& context) const {
    std::vector<ValidationError> errors;
    
    // Check workspace clearance in front of work surfaces
    std::string catalogId = object.getCatalogItemId();
    
    if (catalogId.find("counter") != std::string::npos || 
        catalogId.find("sink") != std::string::npos ||
        catalogId.find("stove") != std::string::npos) {
        
        // Create workspace area in front of object
        const auto& pos = object.getTransform().translation;
        Geometry::BoundingBox workspaceArea(
            Geometry::Point3D(pos.x - 0.3, pos.y + 0.6, pos.z),
            Geometry::Point3D(pos.x + 0.3, pos.y + 0.6 + minWorkspaceClearance_, pos.z + 2.0)
        );
        
        if (context.sceneManager) {
            auto intersectingObjects = context.sceneManager->getObjectsInRegion(workspaceArea);
            
            for (const auto& otherId : intersectingObjects) {
                if (otherId != object.getId()) {
                    errors.emplace_back(ValidationSeverity::Warning,
                                       "Insufficient workspace clearance",
                                       object.getId(),
                                       pos,
                                       "Maintain " + std::to_string(minWorkspaceClearance_) + "m workspace clearance",
                                       getRuleId());
                }
            }
        }
    }
    
    return errors;
}

bool ClearanceValidationRule::hasDoors(const SceneObject& object) const {
    std::string catalogId = object.getCatalogItemId();
    return catalogId.find("cabinet") != std::string::npos ||
           catalogId.find("appliance") != std::string::npos;
}

Geometry::BoundingBox ClearanceValidationRule::getDoorClearanceArea(const SceneObject& object) const {
    const auto& pos = object.getTransform().translation;
    
    // Assume doors open forward (positive Y direction)
    return Geometry::BoundingBox(
        Geometry::Point3D(pos.x - 0.3, pos.y + 0.6, pos.z),
        Geometry::Point3D(pos.x + 0.3, pos.y + 0.6 + minDoorClearance_, pos.z + 2.0)
    );
}

// KitchenModuleValidationRule Implementation

KitchenModuleValidationRule::KitchenModuleValidationRule()
    : BaseValidationRule("kitchen.modules", "Kitchen Module Validation",
                        "Validates kitchen-specific constraints and relationships",
                        ValidationSeverity::Warning) {}

std::vector<ValidationError> KitchenModuleValidationRule::validate(const SceneObject& object,
                                                                  const ValidationContext& context) {
    std::vector<ValidationError> errors;
    
    std::string objectType = getKitchenObjectType(object);
    
    // Validate based on object type
    if (objectType == "sink") {
        auto sinkErrors = validateSinkPlacement(object, context);
        errors.insert(errors.end(), sinkErrors.begin(), sinkErrors.end());
    } else if (objectType == "stove" || objectType == "cooktop") {
        auto stoveErrors = validateStovePlacement(object, context);
        errors.insert(errors.end(), stoveErrors.begin(), stoveErrors.end());
    } else if (objectType == "refrigerator") {
        auto fridgeErrors = validateRefrigeratorPlacement(object, context);
        errors.insert(errors.end(), fridgeErrors.begin(), fridgeErrors.end());
    } else if (objectType.find("cabinet") != std::string::npos) {
        auto cabinetErrors = validateCabinetAlignment(object, context);
        errors.insert(errors.end(), cabinetErrors.begin(), cabinetErrors.end());
    }
    
    // Validate work triangle for major appliances
    if (isMajorAppliance(object)) {
        auto triangleErrors = validateWorkTriangle(object, context);
        errors.insert(errors.end(), triangleErrors.begin(), triangleErrors.end());
    }
    
    return errors;
}

bool KitchenModuleValidationRule::appliesTo(const SceneObject& object) const {
    std::string catalogId = object.getCatalogItemId();
    return catalogId.find("kitchen") != std::string::npos ||
           catalogId.find("cabinet") != std::string::npos ||
           catalogId.find("appliance") != std::string::npos ||
           catalogId.find("sink") != std::string::npos ||
           catalogId.find("stove") != std::string::npos ||
           catalogId.find("refrigerator") != std::string::npos;
}

std::vector<ValidationError> KitchenModuleValidationRule::validateSinkPlacement(const SceneObject& object,
                                                                               const ValidationContext& context) const {
    std::vector<ValidationError> errors;
    
    // Check if sink is near a wall (for plumbing)
    const auto& pos = object.getTransform().translation;
    
    if (context.project) {
        const auto& roomDims = context.project->getDimensions();
        double distToWall = std::min({pos.x, roomDims.width - pos.x, pos.y, roomDims.depth - pos.y});
        
        if (distToWall > 1.0) { // More than 1m from any wall
            errors.emplace_back(ValidationSeverity::Warning,
                               "Sink placed far from walls - consider plumbing requirements",
                               object.getId(),
                               pos,
                               "Place sink closer to walls for easier plumbing installation",
                               getRuleId());
        }
    }
    
    // Check for counter space around sink
    if (context.sceneManager) {
        // Look for counter space on both sides
        // This is a simplified check - real implementation would be more detailed
        auto nearbyObjects = context.sceneManager->findNearbyObjects(object.getId(), 1.0);
        
        bool hasCounterSpace = false;
        for (const auto& nearbyId : nearbyObjects) {
            const SceneObject* nearbyObj = context.sceneManager->getObject(nearbyId);
            if (nearbyObj && nearbyObj->getCatalogItemId().find("counter") != std::string::npos) {
                hasCounterSpace = true;
                break;
            }
        }
        
        if (!hasCounterSpace) {
            errors.emplace_back(ValidationSeverity::Warning,
                               "Sink lacks adjacent counter space",
                               object.getId(),
                               pos,
                               "Add counter space next to sink for food preparation",
                               getRuleId());
        }
    }
    
    return errors;
}

std::vector<ValidationError> KitchenModuleValidationRule::validateStovePlacement(const SceneObject& object,
                                                                                const ValidationContext& context) const {
    std::vector<ValidationError> errors;
    
    const auto& pos = object.getTransform().translation;
    
    // Check clearance from walls and other objects
    if (context.sceneManager) {
        auto nearbyObjects = context.sceneManager->findNearbyObjects(object.getId(), 0.3);
        
        for (const auto& nearbyId : nearbyObjects) {
            const SceneObject* nearbyObj = context.sceneManager->getObject(nearbyId);
            if (nearbyObj) {
                double distance = calculateDistance(object, *nearbyObj);
                if (distance < 0.15) { // 15cm minimum clearance
                    errors.emplace_back(ValidationSeverity::Warning,
                                       "Stove too close to other objects - fire safety concern",
                                       object.getId(),
                                       pos,
                                       "Maintain minimum 15cm clearance around stove",
                                       getRuleId());
                }
            }
        }
    }
    
    return errors;
}

std::vector<ValidationError> KitchenModuleValidationRule::validateRefrigeratorPlacement(const SceneObject& object,
                                                                                       const ValidationContext& context) const {
    std::vector<ValidationError> errors;
    
    // Check door opening clearance
    const auto& pos = object.getTransform().translation;
    
    // Refrigerator needs clearance for door opening (typically 90 degrees)
    if (context.sceneManager) {
        // Check area where door would open
        Geometry::BoundingBox doorArea(
            Geometry::Point3D(pos.x - 0.7, pos.y, pos.z),
            Geometry::Point3D(pos.x, pos.y + 0.7, pos.z + 2.0)
        );
        
        auto intersectingObjects = context.sceneManager->getObjectsInRegion(doorArea);
        
        for (const auto& otherId : intersectingObjects) {
            if (otherId != object.getId()) {
                errors.emplace_back(ValidationSeverity::Warning,
                                   "Refrigerator door opening may be blocked",
                                   object.getId(),
                                   pos,
                                   "Ensure clear space for refrigerator door to open fully",
                                   getRuleId());
            }
        }
    }
    
    return errors;
}

std::vector<ValidationError> KitchenModuleValidationRule::validateCabinetAlignment(const SceneObject& object,
                                                                                  const ValidationContext& context) const {
    std::vector<ValidationError> errors;
    
    // Check alignment with other cabinets
    if (context.sceneManager) {
        auto nearbyObjects = context.sceneManager->findNearbyObjects(object.getId(), 1.0);
        
        for (const auto& nearbyId : nearbyObjects) {
            const SceneObject* nearbyObj = context.sceneManager->getObject(nearbyId);
            if (nearbyObj && nearbyObj->getCatalogItemId().find("cabinet") != std::string::npos) {
                
                const auto& pos1 = object.getTransform().translation;
                const auto& pos2 = nearbyObj->getTransform().translation;
                
                // Check height alignment
                if (std::abs(pos1.z - pos2.z) > 0.02) { // 2cm tolerance
                    errors.emplace_back(ValidationSeverity::Info,
                                       "Cabinet height misalignment with adjacent cabinet",
                                       object.getId(),
                                       pos1,
                                       "Align cabinet heights for consistent appearance",
                                       getRuleId());
                }
            }
        }
    }
    
    return errors;
}

std::vector<ValidationError> KitchenModuleValidationRule::validateWorkTriangle(const SceneObject& object,
                                                                              const ValidationContext& context) const {
    std::vector<ValidationError> errors;
    
    if (!context.sceneManager) {
        return errors;
    }
    
    std::string objectType = getKitchenObjectType(object);
    
    // Find the other two points of the work triangle
    std::vector<const SceneObject*> sinks = findObjectsByType("sink", context);
    std::vector<const SceneObject*> stoves = findObjectsByType("stove", context);
    std::vector<const SceneObject*> refrigerators = findObjectsByType("refrigerator", context);
    
    // Check work triangle distances (should be between 1.2m and 2.7m for each leg)
    if (objectType == "sink" && !stoves.empty() && !refrigerators.empty()) {
        double sinkToStove = calculateDistance(object, *stoves[0]);
        double sinkToFridge = calculateDistance(object, *refrigerators[0]);
        
        if (sinkToStove > constraints_.maxSinkToStoveDistance) {
            errors.emplace_back(ValidationSeverity::Info,
                               "Sink and stove are far apart - may reduce kitchen efficiency",
                               object.getId(),
                               object.getTransform().translation,
                               "Consider moving sink closer to stove for better workflow",
                               getRuleId());
        }
        
        if (sinkToFridge < 1.2 || sinkToFridge > 2.7) {
            errors.emplace_back(ValidationSeverity::Info,
                               "Sink-refrigerator distance outside optimal range (1.2-2.7m)",
                               object.getId(),
                               object.getTransform().translation,
                               "Adjust placement for optimal work triangle",
                               getRuleId());
        }
    }
    
    return errors;
}

std::vector<const SceneObject*> KitchenModuleValidationRule::findObjectsByType(const std::string& type,
                                                                              const ValidationContext& context) const {
    std::vector<const SceneObject*> objects;
    
    if (context.sceneManager) {
        context.sceneManager->forEachObject([&](const ObjectId& id, const SceneObject* obj) {
            if (obj && getKitchenObjectType(*obj) == type) {
                objects.push_back(obj);
            }
        });
    }
    
    return objects;
}

std::string KitchenModuleValidationRule::getKitchenObjectType(const SceneObject& object) const {
    std::string catalogId = object.getCatalogItemId();
    
    if (catalogId.find("sink") != std::string::npos) return "sink";
    if (catalogId.find("stove") != std::string::npos || catalogId.find("cooktop") != std::string::npos) return "stove";
    if (catalogId.find("refrigerator") != std::string::npos || catalogId.find("fridge") != std::string::npos) return "refrigerator";
    if (catalogId.find("cabinet") != std::string::npos) return "cabinet";
    if (catalogId.find("counter") != std::string::npos) return "counter";
    
    return "unknown";
}

bool KitchenModuleValidationRule::isMajorAppliance(const SceneObject& object) const {
    std::string type = getKitchenObjectType(object);
    return type == "sink" || type == "stove" || type == "refrigerator";
}

double KitchenModuleValidationRule::calculateDistance(const SceneObject& objA, const SceneObject& objB) const {
    const auto& posA = objA.getTransform().translation;
    const auto& posB = objB.getTransform().translation;
    return posA.distanceTo(posB);
}

} // namespace Validation
} // namespace KitchenCAD