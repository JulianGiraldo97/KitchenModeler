#include "ValidationService.h"
#include "ValidationRules.h"
#include "../models/Project.h"
#include "../scene/SceneManager.h"
#include "../utils/Logger.h"
#include <algorithm>

namespace KitchenCAD {
namespace Validation {

ValidationService::ValidationService(double tolerance)
    : tolerance_(tolerance), strictMode_(false) {
    initializeDefaultRules();
}

std::vector<ValidationError> ValidationService::validateProject(const Models::Project& project) {
    std::vector<ValidationError> errors;
    
    // Validate project dimensions
    const auto& dimensions = project.getDimensions();
    if (!dimensions.isValid()) {
        errors.emplace_back(ValidationSeverity::Critical, 
                           "Invalid project dimensions", 
                           "", Geometry::Point3D(), 
                           "Ensure all dimensions are positive values", 
                           "project.dimensions");
    }
    
    // Validate walls
    for (const auto& wall : project.getWalls()) {
        if (!wall.isValid()) {
            errors.emplace_back(ValidationSeverity::Error,
                               "Invalid wall: " + wall.id,
                               wall.id, wall.start,
                               "Check wall dimensions and ensure length > 10cm",
                               "wall.dimensions");
        }
    }
    
    // Validate openings
    for (const auto& opening : project.getOpenings()) {
        if (!opening.isValid()) {
            errors.emplace_back(ValidationSeverity::Error,
                               "Invalid opening: " + opening.id,
                               opening.id, Geometry::Point3D(),
                               "Check opening dimensions and position",
                               "opening.dimensions");
        }
        
        // Check if opening's wall exists
        if (!project.getWall(opening.wallId)) {
            errors.emplace_back(ValidationSeverity::Error,
                               "Opening references non-existent wall: " + opening.wallId,
                               opening.id, Geometry::Point3D(),
                               "Ensure the wall exists before adding openings",
                               "opening.wall_reference");
        }
    }
    
    // Validate objects using scene context if available
    ValidationContext context(nullptr, &project);
    context.enableStrictMode = strictMode_;
    context.toleranceDistance = tolerance_;
    
    for (const auto& object : project.getObjects()) {
        if (object) {
            auto objectErrors = validateObject(*object, context);
            errors.insert(errors.end(), objectErrors.begin(), objectErrors.end());
        }
    }
    
    return filterErrors(errors);
}

std::vector<ValidationError> ValidationService::validateObject(const SceneObject& object, 
                                                              const ValidationContext& context) {
    std::vector<ValidationError> errors;
    
    // Basic object validation
    if (object.getId().empty()) {
        errors.emplace_back(ValidationSeverity::Error,
                           "Object has no ID",
                           "", Geometry::Point3D(),
                           "Assign a unique ID to the object",
                           "object.id");
    }
    
    if (object.getCatalogItemId().empty()) {
        errors.emplace_back(ValidationSeverity::Warning,
                           "Object has no catalog item reference",
                           object.getId(), Geometry::Point3D(),
                           "Link object to a catalog item",
                           "object.catalog_reference");
    }
    
    // Apply all validation rules
    auto ruleErrors = applyRules(object, context);
    errors.insert(errors.end(), ruleErrors.begin(), ruleErrors.end());
    
    return filterErrors(errors);
}

std::vector<ValidationError> ValidationService::validatePlacement(const SceneObject& object, 
                                                                 const Geometry::Transform3D& transform,
                                                                 const ValidationContext& context) {
    std::vector<ValidationError> errors;
    
    // Create a temporary context with the new transform
    ValidationContext tempContext = context;
    
    // Validate transform values
    const auto& translation = transform.translation;
    const auto& scale = transform.scale;
    
    // Check for invalid translation
    if (std::isnan(translation.x) || std::isnan(translation.y) || std::isnan(translation.z) ||
        std::isinf(translation.x) || std::isinf(translation.y) || std::isinf(translation.z)) {
        errors.emplace_back(ValidationSeverity::Critical,
                           "Invalid translation values",
                           object.getId(), translation,
                           "Ensure translation coordinates are valid numbers",
                           "placement.translation");
    }
    
    // Check for invalid scale
    if (scale.x <= 0.0 || scale.y <= 0.0 || scale.z <= 0.0 ||
        std::isnan(scale.x) || std::isnan(scale.y) || std::isnan(scale.z)) {
        errors.emplace_back(ValidationSeverity::Error,
                           "Invalid scale values",
                           object.getId(), translation,
                           "Scale values must be positive numbers",
                           "placement.scale");
    }
    
    // Check if placement is within project bounds
    if (context.project) {
        const auto& dimensions = context.project->getDimensions();
        Geometry::BoundingBox projectBounds = dimensions.toBoundingBox();
        
        if (translation.x < projectBounds.min.x || translation.x > projectBounds.max.x ||
            translation.y < projectBounds.min.y || translation.y > projectBounds.max.y ||
            translation.z < projectBounds.min.z || translation.z > projectBounds.max.z) {
            errors.emplace_back(ValidationSeverity::Warning,
                               "Object placed outside project boundaries",
                               object.getId(), translation,
                               "Move object within the defined room dimensions",
                               "placement.bounds");
        }
    }
    
    // Apply placement-specific rules
    for (const auto& [ruleId, rule] : rules_) {
        if (enabledRules_.count(ruleId) && rule->appliesTo(object)) {
            // For placement validation, we need to temporarily modify the object's transform
            // This is a conceptual validation - in practice, we'd need a way to test placement
            auto ruleErrors = rule->validate(object, tempContext);
            errors.insert(errors.end(), ruleErrors.begin(), ruleErrors.end());
        }
    }
    
    return filterErrors(errors);
}

std::vector<ValidationError> ValidationService::validateCompatibility(const SceneObject& objectA,
                                                                     const SceneObject& objectB,
                                                                     const ValidationContext& context) {
    std::vector<ValidationError> errors;
    
    // Check if objects are the same
    if (objectA.getId() == objectB.getId()) {
        return errors; // Same object, no compatibility issues
    }
    
    // Basic compatibility checks
    const auto& transformA = objectA.getTransform();
    const auto& transformB = objectB.getTransform();
    
    // Check minimum distance between objects
    double distance = transformA.translation.distanceTo(transformB.translation);
    if (distance < context.minClearance) {
        errors.emplace_back(ValidationSeverity::Warning,
                           "Objects are too close together",
                           objectA.getId(), transformA.translation,
                           "Maintain minimum clearance of " + std::to_string(context.minClearance) + "m",
                           "compatibility.clearance");
    }
    
    // Check for potential functional conflicts
    // This would be expanded based on specific kitchen module types
    std::string catalogA = objectA.getCatalogItemId();
    std::string catalogB = objectB.getCatalogItemId();
    
    // Example: Check if two sinks are placed too close
    if (catalogA.find("sink") != std::string::npos && catalogB.find("sink") != std::string::npos) {
        if (distance < 1.0) { // 1 meter minimum between sinks
            errors.emplace_back(ValidationSeverity::Warning,
                               "Multiple sinks placed too close together",
                               objectA.getId(), transformA.translation,
                               "Maintain at least 1m distance between sinks",
                               "compatibility.sink_spacing");
        }
    }
    
    return filterErrors(errors);
}

bool ValidationService::isRuleEnabled(const std::string& ruleId) const {
    return enabledRules_.count(ruleId) > 0;
}

void ValidationService::setRuleEnabled(const std::string& ruleId, bool enabled) {
    if (enabled) {
        enabledRules_.insert(ruleId);
    } else {
        enabledRules_.erase(ruleId);
    }
}

std::vector<std::string> ValidationService::getAvailableRules() const {
    std::vector<std::string> ruleIds;
    ruleIds.reserve(rules_.size());
    
    for (const auto& [ruleId, rule] : rules_) {
        ruleIds.push_back(ruleId);
    }
    
    return ruleIds;
}

void ValidationService::addRule(std::unique_ptr<IValidationRule> rule) {
    if (rule) {
        std::string ruleId = rule->getRuleId();
        rules_[ruleId] = std::move(rule);
        enabledRules_.insert(ruleId); // Enable by default
    }
}

bool ValidationService::removeRule(const std::string& ruleId) {
    enabledRules_.erase(ruleId);
    return rules_.erase(ruleId) > 0;
}

IValidationRule* ValidationService::getRule(const std::string& ruleId) const {
    auto it = rules_.find(ruleId);
    return (it != rules_.end()) ? it->second.get() : nullptr;
}

std::vector<ValidationError> ValidationService::validateScene(const SceneManager& sceneManager,
                                                             const Models::Project* project) {
    std::vector<ValidationError> errors;
    
    ValidationContext context(&sceneManager, project);
    context.enableStrictMode = strictMode_;
    context.toleranceDistance = tolerance_;
    
    // Validate all objects in the scene
    sceneManager.forEachObject([&](const ObjectId& id, const SceneObject* object) {
        if (object) {
            auto objectErrors = validateObject(*object, context);
            errors.insert(errors.end(), objectErrors.begin(), objectErrors.end());
        }
    });
    
    // Check for collisions between all objects
    auto allObjects = sceneManager.getAllObjects();
    for (size_t i = 0; i < allObjects.size(); ++i) {
        for (size_t j = i + 1; j < allObjects.size(); ++j) {
            const SceneObject* objA = sceneManager.getObject(allObjects[i]);
            const SceneObject* objB = sceneManager.getObject(allObjects[j]);
            
            if (objA && objB) {
                auto compatErrors = validateCompatibility(*objA, *objB, context);
                errors.insert(errors.end(), compatErrors.begin(), compatErrors.end());
            }
        }
    }
    
    return filterErrors(errors);
}

bool ValidationService::quickValidate(const SceneObject& object, const ValidationContext& context) {
    auto errors = validateObject(object, context);
    
    // Return true if no critical errors or errors found
    return std::none_of(errors.begin(), errors.end(), 
                       [](const ValidationError& error) {
                           return error.isError();
                       });
}

ValidationService::ValidationStatistics ValidationService::getStatistics(const std::vector<ValidationError>& errors) const {
    ValidationStatistics stats;
    stats.totalRules = rules_.size();
    stats.enabledRules = enabledRules_.size();
    stats.totalErrors = 0;
    stats.totalWarnings = 0;
    
    for (const auto& error : errors) {
        if (error.isError()) {
            stats.totalErrors++;
        } else if (error.isWarning()) {
            stats.totalWarnings++;
        }
        
        stats.errorsByRule[error.ruleId]++;
    }
    
    return stats;
}

void ValidationService::initializeDefaultRules() {
    // Add default validation rules for kitchen CAD
    addRule(std::make_unique<CollisionValidationRule>());
    addRule(std::make_unique<DimensionValidationRule>());
    addRule(std::make_unique<HeightValidationRule>());
    addRule(std::make_unique<ClearanceValidationRule>());
    addRule(std::make_unique<KitchenModuleValidationRule>());
}

std::vector<ValidationError> ValidationService::applyRules(const SceneObject& object,
                                                          const ValidationContext& context) {
    std::vector<ValidationError> errors;
    
    for (const auto& [ruleId, rule] : rules_) {
        if (enabledRules_.count(ruleId) && rule->appliesTo(object)) {
            try {
                auto ruleErrors = rule->validate(object, context);
                errors.insert(errors.end(), ruleErrors.begin(), ruleErrors.end());
            } catch (const std::exception& e) {
                // Log rule execution error but continue with other rules
                errors.emplace_back(ValidationSeverity::Error,
                                   "Validation rule '" + ruleId + "' failed: " + e.what(),
                                   object.getId(), Geometry::Point3D(),
                                   "Check rule implementation",
                                   "rule.execution_error");
            }
        }
    }
    
    return errors;
}

void ValidationService::notifyError(const ValidationError& error) {
    if (errorCallback_) {
        errorCallback_(error);
    }
}

std::vector<ValidationError> ValidationService::filterErrors(const std::vector<ValidationError>& errors) const {
    std::vector<ValidationError> filtered;
    
    for (const auto& error : errors) {
        // In strict mode, include all errors
        // In normal mode, might filter out some info-level messages
        if (strictMode_ || error.severity != ValidationSeverity::Info) {
            filtered.push_back(error);
        }
    }
    
    return filtered;
}

} // namespace Validation
} // namespace KitchenCAD