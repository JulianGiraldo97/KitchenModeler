#pragma once

#include "../interfaces/IValidationService.h"
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <functional>

namespace KitchenCAD {
namespace Validation {

/**
 * @brief Main validation service implementation
 * 
 * This service manages all validation rules and provides comprehensive
 * validation for projects, objects, and placements in the Kitchen CAD system.
 */
class ValidationService : public IValidationService {
private:
    // Rule management
    std::unordered_map<std::string, std::unique_ptr<IValidationRule>> rules_;
    std::unordered_set<std::string> enabledRules_;
    
    // Configuration
    double tolerance_;
    bool strictMode_;
    
    // Callbacks for validation events
    std::function<void(const ValidationError&)> errorCallback_;
    
public:
    /**
     * @brief Constructor
     */
    explicit ValidationService(double tolerance = 0.001);
    
    /**
     * @brief Destructor
     */
    virtual ~ValidationService() = default;
    
    // IValidationService interface implementation
    std::vector<ValidationError> validateProject(const Models::Project& project) override;
    std::vector<ValidationError> validateObject(const SceneObject& object, 
                                               const ValidationContext& context) override;
    std::vector<ValidationError> validatePlacement(const SceneObject& object, 
                                                  const Geometry::Transform3D& transform,
                                                  const ValidationContext& context) override;
    std::vector<ValidationError> validateCompatibility(const SceneObject& objectA,
                                                      const SceneObject& objectB,
                                                      const ValidationContext& context) override;
    
    bool isRuleEnabled(const std::string& ruleId) const override;
    void setRuleEnabled(const std::string& ruleId, bool enabled) override;
    std::vector<std::string> getAvailableRules() const override;
    
    void setTolerance(double tolerance) override { tolerance_ = tolerance; }
    double getTolerance() const override { return tolerance_; }
    
    // Additional functionality
    
    /**
     * @brief Add a validation rule to the service
     */
    void addRule(std::unique_ptr<IValidationRule> rule);
    
    /**
     * @brief Remove a validation rule
     */
    bool removeRule(const std::string& ruleId);
    
    /**
     * @brief Get a specific rule by ID
     */
    IValidationRule* getRule(const std::string& ruleId) const;
    
    /**
     * @brief Enable or disable strict mode
     */
    void setStrictMode(bool strict) { strictMode_ = strict; }
    
    /**
     * @brief Check if strict mode is enabled
     */
    bool isStrictMode() const { return strictMode_; }
    
    /**
     * @brief Set callback for validation errors
     */
    void setErrorCallback(std::function<void(const ValidationError&)> callback) {
        errorCallback_ = callback;
    }
    
    /**
     * @brief Validate scene integrity (all objects in scene)
     */
    std::vector<ValidationError> validateScene(const SceneManager& sceneManager,
                                              const Models::Project* project = nullptr);
    
    /**
     * @brief Quick validation check - returns true if no errors found
     */
    bool quickValidate(const SceneObject& object, const ValidationContext& context);
    
    /**
     * @brief Get validation statistics
     */
    struct ValidationStatistics {
        size_t totalRules;
        size_t enabledRules;
        size_t totalErrors;
        size_t totalWarnings;
        std::unordered_map<std::string, size_t> errorsByRule;
    };
    
    ValidationStatistics getStatistics(const std::vector<ValidationError>& errors) const;
    
    /**
     * @brief Initialize with default kitchen CAD validation rules
     */
    void initializeDefaultRules();

private:
    /**
     * @brief Apply all applicable rules to an object
     */
    std::vector<ValidationError> applyRules(const SceneObject& object,
                                           const ValidationContext& context);
    
    /**
     * @brief Notify error callback if set
     */
    void notifyError(const ValidationError& error);
    
    /**
     * @brief Filter errors based on current configuration
     */
    std::vector<ValidationError> filterErrors(const std::vector<ValidationError>& errors) const;
};

} // namespace Validation
} // namespace KitchenCAD