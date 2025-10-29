#pragma once

#include <vector>
#include <string>
#include <memory>
#include "../geometry/Point3D.h"
#include "../geometry/BoundingBox.h"
#include "../geometry/Transform3D.h"

namespace KitchenCAD {

// Forward declarations
namespace Models { class SceneObject; }
namespace Scene { class SceneManager; }
using SceneObject = Models::SceneObject;
using SceneManager = Scene::SceneManager;
namespace Models { class Project; }

/**
 * @brief Validation severity levels
 */
enum class ValidationSeverity {
    Info,       // Informational message
    Warning,    // Warning that should be addressed
    Error,      // Error that prevents proper functionality
    Critical    // Critical error that must be fixed
};

/**
 * @brief Validation error/warning information
 */
struct ValidationError {
    ValidationSeverity severity;
    std::string message;
    std::string objectId;
    Geometry::Point3D location;
    std::string suggestion;
    std::string ruleId;  // Identifier for the validation rule that triggered this
    
    ValidationError(ValidationSeverity sev, const std::string& msg, 
                   const std::string& objId = "", const Geometry::Point3D& loc = Geometry::Point3D(),
                   const std::string& sugg = "", const std::string& rule = "")
        : severity(sev), message(msg), objectId(objId), location(loc), suggestion(sugg), ruleId(rule) {}
    
    bool isError() const { return severity == ValidationSeverity::Error || severity == ValidationSeverity::Critical; }
    bool isWarning() const { return severity == ValidationSeverity::Warning; }
    bool isCritical() const { return severity == ValidationSeverity::Critical; }
};

/**
 * @brief Validation context for providing additional information during validation
 */
struct ValidationContext {
    const SceneManager* sceneManager = nullptr;
    const Models::Project* project = nullptr;
    bool enableStrictMode = false;
    double toleranceDistance = 0.001; // 1mm tolerance
    double minClearance = 0.05; // 5cm minimum clearance
    
    ValidationContext() = default;
    ValidationContext(const SceneManager* scene, const Models::Project* proj = nullptr)
        : sceneManager(scene), project(proj) {}
};

/**
 * @brief Interface for validation services
 */
class IValidationService {
public:
    virtual ~IValidationService() = default;
    
    /**
     * @brief Validate an entire project
     */
    virtual std::vector<ValidationError> validateProject(const Models::Project& project) = 0;
    
    /**
     * @brief Validate a single object in context
     */
    virtual std::vector<ValidationError> validateObject(const SceneObject& object, 
                                                       const ValidationContext& context) = 0;
    
    /**
     * @brief Validate object placement at a specific transform
     */
    virtual std::vector<ValidationError> validatePlacement(const SceneObject& object, 
                                                          const Geometry::Transform3D& transform,
                                                          const ValidationContext& context) = 0;
    
    /**
     * @brief Validate compatibility between two objects
     */
    virtual std::vector<ValidationError> validateCompatibility(const SceneObject& objectA,
                                                              const SceneObject& objectB,
                                                              const ValidationContext& context) = 0;
    
    /**
     * @brief Check if a specific validation rule is enabled
     */
    virtual bool isRuleEnabled(const std::string& ruleId) const = 0;
    
    /**
     * @brief Enable or disable a specific validation rule
     */
    virtual void setRuleEnabled(const std::string& ruleId, bool enabled) = 0;
    
    /**
     * @brief Get all available validation rules
     */
    virtual std::vector<std::string> getAvailableRules() const = 0;
    
    /**
     * @brief Set validation tolerance for distance-based checks
     */
    virtual void setTolerance(double tolerance) = 0;
    
    /**
     * @brief Get current validation tolerance
     */
    virtual double getTolerance() const = 0;
};

/**
 * @brief Interface for specific validation rules
 */
class IValidationRule {
public:
    virtual ~IValidationRule() = default;
    
    /**
     * @brief Get unique identifier for this rule
     */
    virtual std::string getRuleId() const = 0;
    
    /**
     * @brief Get human-readable name for this rule
     */
    virtual std::string getRuleName() const = 0;
    
    /**
     * @brief Get description of what this rule validates
     */
    virtual std::string getDescription() const = 0;
    
    /**
     * @brief Validate an object against this rule
     */
    virtual std::vector<ValidationError> validate(const SceneObject& object,
                                                 const ValidationContext& context) = 0;
    
    /**
     * @brief Check if this rule applies to the given object type
     */
    virtual bool appliesTo(const SceneObject& object) const = 0;
    
    /**
     * @brief Get the severity level for violations of this rule
     */
    virtual ValidationSeverity getSeverity() const = 0;
};

} // namespace KitchenCAD