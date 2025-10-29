#pragma once

#include "../interfaces/IValidationService.h"
#include "../geometry/BoundingBox.h"

namespace KitchenCAD {
namespace Validation {

/**
 * @brief Base class for validation rules
 */
class BaseValidationRule : public IValidationRule {
protected:
    std::string ruleId_;
    std::string ruleName_;
    std::string description_;
    ValidationSeverity severity_;
    
public:
    BaseValidationRule(const std::string& id, const std::string& name, 
                      const std::string& desc, ValidationSeverity sev)
        : ruleId_(id), ruleName_(name), description_(desc), severity_(sev) {}
    
    std::string getRuleId() const override { return ruleId_; }
    std::string getRuleName() const override { return ruleName_; }
    std::string getDescription() const override { return description_; }
    ValidationSeverity getSeverity() const override { return severity_; }
};

/**
 * @brief Collision detection validation rule
 * 
 * Validates that objects don't overlap or collide with each other.
 * Implements requirement 14.2: verificar compatibilidad entre módulos adyacentes
 */
class CollisionValidationRule : public BaseValidationRule {
public:
    CollisionValidationRule();
    
    std::vector<ValidationError> validate(const SceneObject& object,
                                         const ValidationContext& context) override;
    
    bool appliesTo(const SceneObject& object) const override;

private:
    /**
     * @brief Check if two bounding boxes intersect
     */
    bool checkBoundingBoxCollision(const Geometry::BoundingBox& a, const Geometry::BoundingBox& b) const;
    
    /**
     * @brief Calculate object bounding box with transform applied
     */
    Geometry::BoundingBox calculateTransformedBounds(const SceneObject& object) const;
};

/**
 * @brief Dimension validation rule
 * 
 * Validates object dimensions are within acceptable ranges.
 * Implements requirement 14.4: validar que los módulos no excedan las dimensiones del espacio
 */
class DimensionValidationRule : public BaseValidationRule {
private:
    double minDimension_;
    double maxDimension_;
    
public:
    DimensionValidationRule(double minDim = 0.01, double maxDim = 10.0);
    
    std::vector<ValidationError> validate(const SceneObject& object,
                                         const ValidationContext& context) override;
    
    bool appliesTo(const SceneObject& object) const override;
    
    void setDimensionLimits(double minDim, double maxDim) {
        minDimension_ = minDim;
        maxDimension_ = maxDim;
    }

private:
    /**
     * @brief Get object dimensions from catalog or geometry
     */
    Geometry::Vector3D getObjectDimensions(const SceneObject& object) const;
};

/**
 * @brief Height validation rule
 * 
 * Validates object heights are within acceptable ranges for kitchen modules.
 * Implements requirement 14.1: validar altura mínima y máxima permitida
 */
class HeightValidationRule : public BaseValidationRule {
private:
    double minHeight_;
    double maxHeight_;
    double standardCounterHeight_;
    double standardWallCabinetHeight_;
    
public:
    HeightValidationRule();
    
    std::vector<ValidationError> validate(const SceneObject& object,
                                         const ValidationContext& context) override;
    
    bool appliesTo(const SceneObject& object) const override;
    
    void setHeightLimits(double minHeight, double maxHeight) {
        minHeight_ = minHeight;
        maxHeight_ = maxHeight;
    }

private:
    /**
     * @brief Determine expected height based on object type
     */
    double getExpectedHeight(const SceneObject& object) const;
    
    /**
     * @brief Get object type from catalog ID
     */
    std::string getObjectType(const SceneObject& object) const;
    
    /**
     * @brief Get object dimensions from catalog or geometry
     */
    Geometry::Vector3D getObjectDimensions(const SceneObject& object) const;
};

/**
 * @brief Clearance validation rule
 * 
 * Validates minimum clearances for door openings and accessibility.
 * Implements requirement 14.3: alertar sobre espacios insuficientes para apertura de puertas
 */
class ClearanceValidationRule : public BaseValidationRule {
private:
    double minDoorClearance_;
    double minWalkwayClearance_;
    double minWorkspaceClearance_;
    
public:
    ClearanceValidationRule();
    
    std::vector<ValidationError> validate(const SceneObject& object,
                                         const ValidationContext& context) override;
    
    bool appliesTo(const SceneObject& object) const override;
    
    void setClearanceRequirements(double doorClearance, double walkwayClearance, double workspaceClearance) {
        minDoorClearance_ = doorClearance;
        minWalkwayClearance_ = walkwayClearance;
        minWorkspaceClearance_ = workspaceClearance;
    }

private:
    /**
     * @brief Check clearance around doors and drawers
     */
    std::vector<ValidationError> checkDoorClearance(const SceneObject& object,
                                                   const ValidationContext& context) const;
    
    /**
     * @brief Check walkway clearances
     */
    std::vector<ValidationError> checkWalkwayClearance(const SceneObject& object,
                                                      const ValidationContext& context) const;
    
    /**
     * @brief Check workspace clearances
     */
    std::vector<ValidationError> checkWorkspaceClearance(const SceneObject& object,
                                                        const ValidationContext& context) const;
    
    /**
     * @brief Determine if object has doors or drawers
     */
    bool hasDoors(const SceneObject& object) const;
    
    /**
     * @brief Get door opening direction and clearance area
     */
    Geometry::BoundingBox getDoorClearanceArea(const SceneObject& object) const;
};

/**
 * @brief Kitchen module specific validation rule
 * 
 * Validates kitchen-specific constraints and relationships.
 * Implements requirement 14.5: mostrar advertencias visuales para violaciones de restricciones
 */
class KitchenModuleValidationRule : public BaseValidationRule {
private:
    struct KitchenConstraints {
        double maxSinkToStoveDistance = 3.0;  // Maximum distance between sink and stove
        double minCounterSpace = 0.6;         // Minimum counter space between appliances
        double maxReachHeight = 2.2;          // Maximum reachable height
        double minBaseToWallGap = 0.05;       // Minimum gap between base and wall cabinets
        double standardCounterDepth = 0.6;    // Standard counter depth
    } constraints_;
    
public:
    KitchenModuleValidationRule();
    
    std::vector<ValidationError> validate(const SceneObject& object,
                                         const ValidationContext& context) override;
    
    bool appliesTo(const SceneObject& object) const override;

private:
    /**
     * @brief Validate sink placement and requirements
     */
    std::vector<ValidationError> validateSinkPlacement(const SceneObject& object,
                                                      const ValidationContext& context) const;
    
    /**
     * @brief Validate stove/cooktop placement
     */
    std::vector<ValidationError> validateStovePlacement(const SceneObject& object,
                                                       const ValidationContext& context) const;
    
    /**
     * @brief Validate refrigerator placement
     */
    std::vector<ValidationError> validateRefrigeratorPlacement(const SceneObject& object,
                                                              const ValidationContext& context) const;
    
    /**
     * @brief Validate cabinet alignment and spacing
     */
    std::vector<ValidationError> validateCabinetAlignment(const SceneObject& object,
                                                         const ValidationContext& context) const;
    
    /**
     * @brief Check work triangle efficiency (sink-stove-refrigerator)
     */
    std::vector<ValidationError> validateWorkTriangle(const SceneObject& object,
                                                     const ValidationContext& context) const;
    
    /**
     * @brief Find objects of specific type in scene
     */
    std::vector<const SceneObject*> findObjectsByType(const std::string& type,
                                                     const ValidationContext& context) const;
    
    /**
     * @brief Get object type from catalog ID
     */
    std::string getKitchenObjectType(const SceneObject& object) const;
    
    /**
     * @brief Check if object is a major appliance
     */
    bool isMajorAppliance(const SceneObject& object) const;
    
    /**
     * @brief Calculate distance between two objects
     */
    double calculateDistance(const SceneObject& objA, const SceneObject& objB) const;
};

} // namespace Validation
} // namespace KitchenCAD