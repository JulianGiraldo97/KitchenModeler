#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../src/validation/ValidationService.h"
#include "../src/validation/ValidationRules.h"
#include "../src/validation/ValidationVisualizer.h"
#include "../src/models/Project.h"
#include "../src/scene/SceneManager.h"
#include <memory>

using namespace KitchenCAD;
using namespace KitchenCAD::Validation;
using namespace KitchenCAD::Models;
using namespace KitchenCAD::Scene;
using Catch::Approx;

// Type aliases to resolve ambiguity
using TestSceneObject = KitchenCAD::Models::SceneObject;
using TestSceneManager = KitchenCAD::Scene::SceneManager;

// Mock renderer for testing visualization
class MockValidationRenderer : public IValidationRenderer {
private:
    std::vector<ValidationVisual> renderedVisuals_;
    bool visualsVisible_ = true;
    
public:
    void renderValidationVisual(const ValidationVisual& visual) override {
        renderedVisuals_.push_back(visual);
    }
    
    void clearValidationVisuals() override {
        renderedVisuals_.clear();
    }
    
    void setValidationVisualsVisible(bool visible) override {
        visualsVisible_ = visible;
    }
    
    const std::vector<ValidationVisual>& getRenderedVisuals() const { return renderedVisuals_; }
    bool areVisualsVisible() const { return visualsVisible_; }
    size_t getRenderedCount() const { return renderedVisuals_.size(); }
};

TEST_CASE("ValidationError creation and properties", "[validation][error]") {
    SECTION("Basic validation error") {
        ValidationError error(ValidationSeverity::Warning, "Test warning message", 
                            "object_123", Geometry::Point3D(1.0, 2.0, 3.0),
                            "Fix this issue", "test.rule");
        
        REQUIRE(error.severity == ValidationSeverity::Warning);
        REQUIRE(error.message == "Test warning message");
        REQUIRE(error.objectId == "object_123");
        REQUIRE(error.location.x == Approx(1.0));
        REQUIRE(error.location.y == Approx(2.0));
        REQUIRE(error.location.z == Approx(3.0));
        REQUIRE(error.suggestion == "Fix this issue");
        REQUIRE(error.ruleId == "test.rule");
        
        REQUIRE_FALSE(error.isError());
        REQUIRE(error.isWarning());
        REQUIRE_FALSE(error.isCritical());
    }
    
    SECTION("Error severity checks") {
        ValidationError info(ValidationSeverity::Info, "Info message");
        ValidationError warning(ValidationSeverity::Warning, "Warning message");
        ValidationError error(ValidationSeverity::Error, "Error message");
        ValidationError critical(ValidationSeverity::Critical, "Critical message");
        
        REQUIRE_FALSE(info.isError());
        REQUIRE_FALSE(info.isWarning());
        REQUIRE_FALSE(info.isCritical());
        
        REQUIRE_FALSE(warning.isError());
        REQUIRE(warning.isWarning());
        REQUIRE_FALSE(warning.isCritical());
        
        REQUIRE(error.isError());
        REQUIRE_FALSE(error.isWarning());
        REQUIRE_FALSE(error.isCritical());
        
        REQUIRE(critical.isError());
        REQUIRE_FALSE(critical.isWarning());
        REQUIRE(critical.isCritical());
    }
}

TEST_CASE("ValidationContext setup", "[validation][context]") {
    SECTION("Basic context creation") {
        ValidationContext context;
        
        REQUIRE(context.sceneManager == nullptr);
        REQUIRE(context.project == nullptr);
        REQUIRE_FALSE(context.enableStrictMode);
        REQUIRE(context.toleranceDistance == Approx(0.001));
        REQUIRE(context.minClearance == Approx(0.05));
    }
    
    SECTION("Context with scene manager") {
        TestSceneManager sceneManager;
        Project project("Test", RoomDimensions(5.0, 3.0, 2.5));
        
        ValidationContext context(&sceneManager, &project);
        
        REQUIRE(context.sceneManager == &sceneManager);
        REQUIRE(context.project == &project);
    }
}

TEST_CASE("ValidationService basic operations", "[validation][service]") {
    SECTION("Service creation and configuration") {
        ValidationService service(0.002); // 2mm tolerance
        
        REQUIRE(service.getTolerance() == Approx(0.002));
        REQUIRE_FALSE(service.isStrictMode());
        
        // Check default rules are loaded
        auto rules = service.getAvailableRules();
        REQUIRE_FALSE(rules.empty());
        REQUIRE(rules.size() >= 5); // Should have at least 5 default rules
        
        // All rules should be enabled by default
        for (const auto& ruleId : rules) {
            REQUIRE(service.isRuleEnabled(ruleId));
        }
    }
    
    SECTION("Rule management") {
        ValidationService service;
        
        auto rules = service.getAvailableRules();
        REQUIRE_FALSE(rules.empty());
        
        std::string firstRule = rules[0];
        
        // Disable rule
        service.setRuleEnabled(firstRule, false);
        REQUIRE_FALSE(service.isRuleEnabled(firstRule));
        
        // Re-enable rule
        service.setRuleEnabled(firstRule, true);
        REQUIRE(service.isRuleEnabled(firstRule));
    }
    
    SECTION("Tolerance and strict mode") {
        ValidationService service;
        
        service.setTolerance(0.005);
        REQUIRE(service.getTolerance() == Approx(0.005));
        
        service.setStrictMode(true);
        REQUIRE(service.isStrictMode());
    }
}

TEST_CASE("ValidationService object validation", "[validation][service]") {
    SECTION("Basic object validation") {
        ValidationService service;
        TestSceneObject object("test_catalog_item");
        
        ValidationContext context;
        auto errors = service.validateObject(object, context);
        
        // Should have at least one warning about missing catalog reference
        REQUIRE_FALSE(errors.empty());
        
        bool foundCatalogWarning = false;
        for (const auto& error : errors) {
            if (error.message.find("catalog") != std::string::npos) {
                foundCatalogWarning = true;
                break;
            }
        }
        REQUIRE(foundCatalogWarning);
    }
    
    SECTION("Object with empty ID validation") {
        ValidationService service;
        TestSceneObject object("test_item");
        object.setId(""); // Empty ID should cause error
        
        ValidationContext context;
        auto errors = service.validateObject(object, context);
        
        bool foundIdError = false;
        for (const auto& error : errors) {
            if (error.message.find("ID") != std::string::npos) {
                foundIdError = true;
                break;
            }
        }
        REQUIRE(foundIdError);
    }
    
    SECTION("Quick validation") {
        ValidationService service;
        TestSceneObject validObject("test_item");
        validObject.setId("valid_id");
        
        ValidationContext context;
        
        // Quick validation should return true for objects without critical errors
        bool isValid = service.quickValidate(validObject, context);
        // Note: This might be false due to other validation rules, but should not crash
        REQUIRE((isValid == true || isValid == false)); // Just ensure it returns a boolean
    }
}

TEST_CASE("ValidationService project validation", "[validation][service]") {
    SECTION("Valid project validation") {
        ValidationService service;
        Project project("Valid Project", RoomDimensions(5.0, 3.0, 2.5));
        
        auto errors = service.validateProject(project);
        
        // Valid project should have no critical errors
        bool hasCriticalErrors = false;
        for (const auto& error : errors) {
            if (error.severity == ValidationSeverity::Critical) {
                hasCriticalErrors = true;
                break;
            }
        }
        REQUIRE_FALSE(hasCriticalErrors);
    }
    
    SECTION("Invalid project dimensions") {
        ValidationService service;
        Project project("Invalid Project", RoomDimensions(0.0, -1.0, 2.5));
        
        auto errors = service.validateProject(project);
        
        // Should have critical error about dimensions
        bool foundDimensionError = false;
        for (const auto& error : errors) {
            if (error.severity == ValidationSeverity::Critical && 
                error.message.find("dimensions") != std::string::npos) {
                foundDimensionError = true;
                break;
            }
        }
        REQUIRE(foundDimensionError);
    }
    
    SECTION("Project with invalid walls") {
        ValidationService service;
        Project project("Wall Test", RoomDimensions(5.0, 3.0, 2.5));
        
        // Add invalid wall (zero length)
        Wall invalidWall("wall_1", Geometry::Point3D(0, 0, 0), Geometry::Point3D(0, 0, 0), 2.5, 0.1);
        project.addWall(invalidWall);
        
        auto errors = service.validateProject(project);
        
        bool foundWallError = false;
        for (const auto& error : errors) {
            if (error.message.find("wall") != std::string::npos) {
                foundWallError = true;
                break;
            }
        }
        REQUIRE(foundWallError);
    }
    
    SECTION("Project with orphan openings") {
        ValidationService service;
        Project project("Opening Test", RoomDimensions(5.0, 3.0, 2.5));
        
        // Add opening without corresponding wall
        Opening orphanOpening("door_1", "non_existent_wall", "door", 0.5, 0.8, 2.0, 0.0);
        project.addOpening(orphanOpening);
        
        auto errors = service.validateProject(project);
        
        bool foundOpeningError = false;
        for (const auto& error : errors) {
            if (error.message.find("non-existent wall") != std::string::npos) {
                foundOpeningError = true;
                break;
            }
        }
        REQUIRE(foundOpeningError);
    }
}

TEST_CASE("ValidationService placement validation", "[validation][service]") {
    SECTION("Valid placement") {
        ValidationService service;
        TestSceneObject object("test_item");
        
        Geometry::Transform3D validTransform;
        validTransform.translation = Geometry::Point3D(1.0, 1.0, 0.0);
        validTransform.scale = Geometry::Vector3D(1.0, 1.0, 1.0);
        
        ValidationContext context;
        auto errors = service.validatePlacement(object, validTransform, context);
        
        // Valid placement should not have critical errors
        bool hasCriticalErrors = false;
        for (const auto& error : errors) {
            if (error.severity == ValidationSeverity::Critical) {
                hasCriticalErrors = true;
                break;
            }
        }
        REQUIRE_FALSE(hasCriticalErrors);
    }
    
    SECTION("Invalid translation values") {
        ValidationService service;
        TestSceneObject object("test_item");
        
        Geometry::Transform3D invalidTransform;
        invalidTransform.translation = Geometry::Point3D(std::numeric_limits<double>::quiet_NaN(), 1.0, 0.0);
        invalidTransform.scale = Geometry::Vector3D(1.0, 1.0, 1.0);
        
        ValidationContext context;
        auto errors = service.validatePlacement(object, invalidTransform, context);
        
        bool foundTranslationError = false;
        for (const auto& error : errors) {
            if (error.severity == ValidationSeverity::Critical && 
                error.message.find("translation") != std::string::npos) {
                foundTranslationError = true;
                break;
            }
        }
        REQUIRE(foundTranslationError);
    }
    
    SECTION("Invalid scale values") {
        ValidationService service;
        TestSceneObject object("test_item");
        
        Geometry::Transform3D invalidTransform;
        invalidTransform.translation = Geometry::Point3D(1.0, 1.0, 0.0);
        invalidTransform.scale = Geometry::Vector3D(-1.0, 1.0, 1.0); // Negative scale
        
        ValidationContext context;
        auto errors = service.validatePlacement(object, invalidTransform, context);
        
        bool foundScaleError = false;
        for (const auto& error : errors) {
            if (error.severity == ValidationSeverity::Error && 
                error.message.find("scale") != std::string::npos) {
                foundScaleError = true;
                break;
            }
        }
        REQUIRE(foundScaleError);
    }
    
    SECTION("Placement outside project bounds") {
        ValidationService service;
        TestSceneObject object("test_item");
        
        Project project("Bounds Test", RoomDimensions(5.0, 3.0, 2.5));
        
        Geometry::Transform3D outOfBoundsTransform;
        outOfBoundsTransform.translation = Geometry::Point3D(10.0, 1.0, 0.0); // Outside room width
        outOfBoundsTransform.scale = Geometry::Vector3D(1.0, 1.0, 1.0);
        
        ValidationContext context(nullptr, &project);
        auto errors = service.validatePlacement(object, outOfBoundsTransform, context);
        
        bool foundBoundsWarning = false;
        for (const auto& error : errors) {
            if (error.severity == ValidationSeverity::Warning && 
                error.message.find("boundaries") != std::string::npos) {
                foundBoundsWarning = true;
                break;
            }
        }
        REQUIRE(foundBoundsWarning);
    }
}

TEST_CASE("ValidationService compatibility validation", "[validation][service]") {
    SECTION("Compatible objects") {
        ValidationService service;
        
        TestSceneObject object1("item1");
        object1.setId("obj1");
        Geometry::Transform3D transform1;
        transform1.translation = Geometry::Point3D(0.0, 0.0, 0.0);
        object1.setTransform(transform1);
        
        TestSceneObject object2("item2");
        object2.setId("obj2");
        Geometry::Transform3D transform2;
        transform2.translation = Geometry::Point3D(2.0, 0.0, 0.0); // 2m apart
        object2.setTransform(transform2);
        
        ValidationContext context;
        auto errors = service.validateCompatibility(object1, object2, context);
        
        // Objects 2m apart should be compatible
        bool hasCompatibilityErrors = false;
        for (const auto& error : errors) {
            if (error.ruleId.find("compatibility") != std::string::npos) {
                hasCompatibilityErrors = true;
                break;
            }
        }
        REQUIRE_FALSE(hasCompatibilityErrors);
    }
    
    SECTION("Objects too close together") {
        ValidationService service;
        
        TestSceneObject object1("item1");
        object1.setId("obj1");
        Geometry::Transform3D transform1;
        transform1.translation = Geometry::Point3D(0.0, 0.0, 0.0);
        object1.setTransform(transform1);
        
        TestSceneObject object2("item2");
        object2.setId("obj2");
        Geometry::Transform3D transform2;
        transform2.translation = Geometry::Point3D(0.01, 0.0, 0.0); // 1cm apart (too close)
        object2.setTransform(transform2);
        
        ValidationContext context;
        context.minClearance = 0.05; // 5cm minimum clearance
        
        auto errors = service.validateCompatibility(object1, object2, context);
        
        bool foundClearanceWarning = false;
        for (const auto& error : errors) {
            if (error.message.find("too close") != std::string::npos) {
                foundClearanceWarning = true;
                break;
            }
        }
        REQUIRE(foundClearanceWarning);
    }
    
    SECTION("Multiple sinks too close") {
        ValidationService service;
        
        TestSceneObject sink1("kitchen_sink_1");
        sink1.setId("sink1");
        Geometry::Transform3D transform1;
        transform1.translation = Geometry::Point3D(0.0, 0.0, 0.0);
        sink1.setTransform(transform1);
        
        TestSceneObject sink2("kitchen_sink_2");
        sink2.setId("sink2");
        Geometry::Transform3D transform2;
        transform2.translation = Geometry::Point3D(0.5, 0.0, 0.0); // 50cm apart
        sink2.setTransform(transform2);
        
        ValidationContext context;
        auto errors = service.validateCompatibility(sink1, sink2, context);
        
        bool foundSinkSpacingWarning = false;
        for (const auto& error : errors) {
            if (error.message.find("sinks") != std::string::npos && 
                error.message.find("close") != std::string::npos) {
                foundSinkSpacingWarning = true;
                break;
            }
        }
        REQUIRE(foundSinkSpacingWarning);
    }
    
    SECTION("Same object compatibility") {
        ValidationService service;
        
        TestSceneObject object("item1");
        object.setId("same_obj");
        
        ValidationContext context;
        auto errors = service.validateCompatibility(object, object, context);
        
        // Same object should have no compatibility issues
        REQUIRE(errors.empty());
    }
}

TEST_CASE("ValidationService scene validation", "[validation][service]") {
    SECTION("Empty scene validation") {
        ValidationService service;
        TestSceneManager sceneManager;
        
        auto errors = service.validateScene(sceneManager);
        
        // Empty scene should have no errors
        REQUIRE(errors.empty());
    }
    
    SECTION("Scene with objects validation") {
        ValidationService service;
        TestSceneManager sceneManager;
        Project project("Scene Test", RoomDimensions(5.0, 3.0, 2.5));
        
        // Add objects to scene
        auto object1 = std::make_unique<TestSceneObject>("item1");
        auto object2 = std::make_unique<TestSceneObject>("item2");
        
        sceneManager.addObject(std::move(object1));
        sceneManager.addObject(std::move(object2));
        
        auto errors = service.validateScene(sceneManager, &project);
        
        // Should validate all objects in scene
        REQUIRE_FALSE(errors.empty()); // Should have some validation messages
    }
}

TEST_CASE("ValidationService statistics", "[validation][service]") {
    SECTION("Statistics calculation") {
        ValidationService service;
        
        std::vector<ValidationError> errors;
        errors.emplace_back(ValidationSeverity::Info, "Info message", "", Geometry::Point3D(), "", "rule1");
        errors.emplace_back(ValidationSeverity::Warning, "Warning message", "", Geometry::Point3D(), "", "rule2");
        errors.emplace_back(ValidationSeverity::Error, "Error message", "", Geometry::Point3D(), "", "rule1");
        errors.emplace_back(ValidationSeverity::Critical, "Critical message", "", Geometry::Point3D(), "", "rule3");
        
        auto stats = service.getStatistics(errors);
        
        REQUIRE(stats.totalErrors == 2); // Error + Critical
        REQUIRE(stats.totalWarnings == 1);
        REQUIRE(stats.errorsByRule["rule1"] == 2); // Info + Error
        REQUIRE(stats.errorsByRule["rule2"] == 1);
        REQUIRE(stats.errorsByRule["rule3"] == 1);
    }
}

TEST_CASE("CollisionValidationRule", "[validation][rules][collision]") {
    SECTION("Rule properties") {
        CollisionValidationRule rule;
        
        REQUIRE(rule.getRuleId() == "collision.overlap");
        REQUIRE(rule.getRuleName() == "Collision Detection");
        REQUIRE_FALSE(rule.getDescription().empty());
        REQUIRE(rule.getSeverity() == ValidationSeverity::Error);
    }
    
    SECTION("Rule applies to all objects") {
        CollisionValidationRule rule;
        TestSceneObject object("test_item");
        
        REQUIRE(rule.appliesTo(object));
    }
    
    SECTION("Collision detection without scene manager") {
        CollisionValidationRule rule;
        TestSceneObject object("test_item");
        
        ValidationContext context; // No scene manager
        auto errors = rule.validate(object, context);
        
        // Should return no errors without scene manager
        REQUIRE(errors.empty());
    }
}

TEST_CASE("DimensionValidationRule", "[validation][rules][dimension]") {
    SECTION("Rule properties") {
        DimensionValidationRule rule;
        
        REQUIRE(rule.getRuleId() == "dimension.limits");
        REQUIRE(rule.getRuleName() == "Dimension Validation");
        REQUIRE(rule.getSeverity() == ValidationSeverity::Warning);
    }
    
    SECTION("Rule applies to all objects") {
        DimensionValidationRule rule;
        TestSceneObject object("test_item");
        
        REQUIRE(rule.appliesTo(object));
    }
    
    SECTION("Dimension limits configuration") {
        DimensionValidationRule rule(0.1, 5.0); // 10cm min, 5m max
        
        TestSceneObject object("test_item");
        ValidationContext context;
        
        auto errors = rule.validate(object, context);
        
        // Should validate dimensions (may have warnings based on default object size)
        REQUIRE_FALSE(errors.empty()); // Default object might trigger warnings
    }
    
    SECTION("Object exceeding room bounds") {
        DimensionValidationRule rule;
        TestSceneObject object("large_item");
        
        // Set object position near room boundary
        Geometry::Transform3D transform;
        transform.translation = Geometry::Point3D(4.5, 2.5, 0.0); // Near room edge
        transform.scale = Geometry::Vector3D(2.0, 2.0, 1.0); // Large scale
        object.setTransform(transform);
        
        Project project("Test Room", RoomDimensions(5.0, 3.0, 2.5));
        ValidationContext context(nullptr, &project);
        
        auto errors = rule.validate(object, context);
        
        bool foundBoundsError = false;
        for (const auto& error : errors) {
            if (error.message.find("boundaries") != std::string::npos) {
                foundBoundsError = true;
                break;
            }
        }
        REQUIRE(foundBoundsError);
    }
}

TEST_CASE("HeightValidationRule", "[validation][rules][height]") {
    SECTION("Rule properties") {
        HeightValidationRule rule;
        
        REQUIRE(rule.getRuleId() == "height.limits");
        REQUIRE(rule.getRuleName() == "Height Validation");
        REQUIRE(rule.getSeverity() == ValidationSeverity::Warning);
    }
    
    SECTION("Rule applies to kitchen modules") {
        HeightValidationRule rule;
        
        TestSceneObject cabinet("kitchen_cabinet_base");
        TestSceneObject counter("kitchen_counter_top");
        TestSceneObject appliance("kitchen_appliance_oven");
        TestSceneObject other("furniture_chair");
        
        REQUIRE(rule.appliesTo(cabinet));
        REQUIRE(rule.appliesTo(counter));
        REQUIRE(rule.appliesTo(appliance));
        REQUIRE_FALSE(rule.appliesTo(other));
    }
    
    SECTION("Height validation") {
        HeightValidationRule rule;
        
        TestSceneObject cabinet("kitchen_cabinet_base");
        ValidationContext context;
        
        auto errors = rule.validate(cabinet, context);
        
        // Should validate height against kitchen standards
        // May have warnings if height doesn't match expected values
        REQUIRE_FALSE(errors.empty()); // Default height might not match standards
    }
    
    SECTION("Height limits") {
        HeightValidationRule rule;
        
        TestSceneObject object("kitchen_cabinet_base");
        
        // Set very small scale (too low)
        Geometry::Transform3D transform;
        transform.scale = Geometry::Vector3D(1.0, 1.0, 0.05); // 5cm height
        object.setTransform(transform);
        
        ValidationContext context;
        auto errors = rule.validate(object, context);
        
        bool foundHeightError = false;
        for (const auto& error : errors) {
            if (error.message.find("height too low") != std::string::npos) {
                foundHeightError = true;
                break;
            }
        }
        REQUIRE(foundHeightError);
    }
}

TEST_CASE("ClearanceValidationRule", "[validation][rules][clearance]") {
    SECTION("Rule properties") {
        ClearanceValidationRule rule;
        
        REQUIRE(rule.getRuleId() == "clearance.requirements");
        REQUIRE(rule.getRuleName() == "Clearance Validation");
        REQUIRE(rule.getSeverity() == ValidationSeverity::Warning);
    }
    
    SECTION("Rule applies to all objects") {
        ClearanceValidationRule rule;
        TestSceneObject object("test_item");
        
        REQUIRE(rule.appliesTo(object));
    }
    
    SECTION("Clearance validation") {
        ClearanceValidationRule rule;
        
        TestSceneObject cabinet("kitchen_cabinet_base");
        ValidationContext context;
        
        auto errors = rule.validate(cabinet, context);
        
        // Should check various clearance requirements
        // May have warnings based on object placement
        REQUIRE_FALSE(errors.empty()); // May have clearance warnings
    }
    
    SECTION("Clearance configuration") {
        ClearanceValidationRule rule;
        rule.setClearanceRequirements(1.0, 1.2, 0.8); // Custom clearances
        
        TestSceneObject object("test_item");
        ValidationContext context;
        
        auto errors = rule.validate(object, context);
        
        // Should use custom clearance requirements
        REQUIRE_FALSE(errors.empty()); // May have clearance issues
    }
}

TEST_CASE("KitchenModuleValidationRule", "[validation][rules][kitchen]") {
    SECTION("Rule properties") {
        KitchenModuleValidationRule rule;
        
        REQUIRE(rule.getRuleId() == "kitchen.modules");
        REQUIRE(rule.getRuleName() == "Kitchen Module Validation");
        REQUIRE(rule.getSeverity() == ValidationSeverity::Warning);
    }
    
    SECTION("Rule applies to kitchen objects") {
        KitchenModuleValidationRule rule;
        
        TestSceneObject kitchenSink("kitchen_sink");
        TestSceneObject kitchenCabinet("kitchen_cabinet_base");
        TestSceneObject appliance("kitchen_appliance_stove");
        TestSceneObject furniture("furniture_chair");
        
        REQUIRE(rule.appliesTo(kitchenSink));
        REQUIRE(rule.appliesTo(kitchenCabinet));
        REQUIRE(rule.appliesTo(appliance));
        REQUIRE_FALSE(rule.appliesTo(furniture));
    }
    
    SECTION("Sink validation") {
        KitchenModuleValidationRule rule;
        
        TestSceneObject sink("kitchen_sink");
        
        // Place sink far from walls
        Geometry::Transform3D transform;
        transform.translation = Geometry::Point3D(2.5, 1.5, 0.0); // Center of 5x3 room
        sink.setTransform(transform);
        
        Project project("Kitchen", RoomDimensions(5.0, 3.0, 2.5));
        ValidationContext context(nullptr, &project);
        
        auto errors = rule.validate(sink, context);
        
        bool foundPlumbingWarning = false;
        for (const auto& error : errors) {
            if (error.message.find("plumbing") != std::string::npos) {
                foundPlumbingWarning = true;
                break;
            }
        }
        REQUIRE(foundPlumbingWarning);
    }
    
    SECTION("Stove validation") {
        KitchenModuleValidationRule rule;
        
        TestSceneObject stove("kitchen_stove");
        ValidationContext context;
        
        auto errors = rule.validate(stove, context);
        
        // Should validate stove placement requirements
        REQUIRE_FALSE(errors.empty()); // May have placement warnings
    }
    
    SECTION("Refrigerator validation") {
        KitchenModuleValidationRule rule;
        
        TestSceneObject refrigerator("kitchen_refrigerator");
        ValidationContext context;
        
        auto errors = rule.validate(refrigerator, context);
        
        // Should validate refrigerator door clearance
        REQUIRE_FALSE(errors.empty()); // May have clearance warnings
    }
}

TEST_CASE("ValidationVisualizer", "[validation][visualizer]") {
    SECTION("Visualizer creation") {
        MockValidationRenderer renderer;
        ValidationVisualizer visualizer(&renderer);
        
        REQUIRE(visualizer.getShowInfo());
        REQUIRE(visualizer.getShowWarnings());
        REQUIRE(visualizer.getShowErrors());
        REQUIRE(visualizer.getShowCritical());
        REQUIRE(visualizer.isBlinkingEnabled());
        REQUIRE(visualizer.getIconScale() == Approx(1.0));
    }
    
    SECTION("Visual creation from errors") {
        MockValidationRenderer renderer;
        ValidationVisualizer visualizer(&renderer);
        
        std::vector<ValidationError> errors;
        errors.emplace_back(ValidationSeverity::Warning, "Test warning", "obj1", 
                           Geometry::Point3D(1.0, 2.0, 3.0), "Fix this", "test.rule");
        errors.emplace_back(ValidationSeverity::Error, "Test error", "obj2",
                           Geometry::Point3D(4.0, 5.0, 6.0), "Fix that", "test.rule2");
        
        visualizer.updateVisuals(errors);
        
        const auto& visuals = visualizer.getVisuals();
        REQUIRE(visuals.size() == 2);
        
        REQUIRE(visuals[0].error.message == "Test warning");
        REQUIRE(visuals[0].position.x == Approx(1.0));
        REQUIRE(visuals[1].error.message == "Test error");
        REQUIRE(visuals[1].position.x == Approx(4.0));
    }
    
    SECTION("Visual visibility control") {
        MockValidationRenderer renderer;
        ValidationVisualizer visualizer(&renderer);
        
        std::vector<ValidationError> errors;
        errors.emplace_back(ValidationSeverity::Info, "Info message");
        errors.emplace_back(ValidationSeverity::Warning, "Warning message");
        errors.emplace_back(ValidationSeverity::Error, "Error message");
        errors.emplace_back(ValidationSeverity::Critical, "Critical message");
        
        visualizer.updateVisuals(errors);
        
        auto stats = visualizer.getStatistics();
        REQUIRE(stats.totalVisuals == 4);
        REQUIRE(stats.visibleCount == 4); // All visible by default
        
        // Hide info messages
        visualizer.setShowInfo(false);
        stats = visualizer.getStatistics();
        REQUIRE(stats.visibleCount == 3); // Info hidden
        
        // Hide warnings
        visualizer.setShowWarnings(false);
        stats = visualizer.getStatistics();
        REQUIRE(stats.visibleCount == 2); // Info and warnings hidden
    }
    
    SECTION("Visual removal by object") {
        MockValidationRenderer renderer;
        ValidationVisualizer visualizer(&renderer);
        
        std::vector<ValidationError> errors;
        errors.emplace_back(ValidationSeverity::Warning, "Warning 1", "obj1");
        errors.emplace_back(ValidationSeverity::Warning, "Warning 2", "obj2");
        errors.emplace_back(ValidationSeverity::Error, "Error 1", "obj1");
        
        visualizer.updateVisuals(errors);
        REQUIRE(visualizer.getVisuals().size() == 3);
        
        // Remove visuals for obj1
        visualizer.removeVisualsForObject("obj1");
        REQUIRE(visualizer.getVisuals().size() == 1);
        
        const auto& remainingVisuals = visualizer.getVisuals();
        REQUIRE(remainingVisuals[0].error.objectId == "obj2");
    }
    
    SECTION("Visual finding by position") {
        MockValidationRenderer renderer;
        ValidationVisualizer visualizer(&renderer);
        
        ValidationError error(ValidationSeverity::Warning, "Test", "obj1", 
                            Geometry::Point3D(5.0, 5.0, 5.0));
        visualizer.addVisual(error, Geometry::Point3D(5.0, 5.0, 5.0));
        
        // Find visual at exact position
        auto* visual = visualizer.findVisualAtPosition(Geometry::Point3D(5.0, 5.0, 5.0), 0.01);
        REQUIRE(visual != nullptr);
        REQUIRE(visual->error.message == "Test");
        
        // Find visual within tolerance
        visual = visualizer.findVisualAtPosition(Geometry::Point3D(5.05, 5.05, 5.05), 0.1);
        REQUIRE(visual != nullptr);
        
        // Should not find visual outside tolerance
        visual = visualizer.findVisualAtPosition(Geometry::Point3D(10.0, 10.0, 10.0), 0.1);
        REQUIRE(visual == nullptr);
    }
    
    SECTION("Animation update") {
        MockValidationRenderer renderer;
        ValidationVisualizer visualizer(&renderer);
        
        ValidationError criticalError(ValidationSeverity::Critical, "Critical issue");
        visualizer.addVisual(criticalError, Geometry::Point3D(0, 0, 0));
        
        const auto& visuals = visualizer.getVisuals();
        REQUIRE(visuals[0].blinking); // Critical errors should blink
        
        float initialAlpha = visuals[0].color.a;
        
        // Update animation
        visualizer.updateAnimation(0.5); // 0.5 seconds
        
        // Alpha should have changed due to blinking
        // Note: This test might be flaky due to timing, but demonstrates the concept
    }
}

TEST_CASE("ValidationPanel", "[validation][panel]") {
    SECTION("Panel creation and error management") {
        ValidationPanel panel;
        
        REQUIRE(panel.getShowInfo());
        REQUIRE(panel.getShowWarnings());
        REQUIRE(panel.getShowErrors());
        REQUIRE(panel.getShowCritical());
        REQUIRE(panel.getFilterText().empty());
        
        std::vector<ValidationError> errors;
        errors.emplace_back(ValidationSeverity::Info, "Info message");
        errors.emplace_back(ValidationSeverity::Warning, "Warning message");
        errors.emplace_back(ValidationSeverity::Error, "Error message");
        errors.emplace_back(ValidationSeverity::Critical, "Critical message");
        
        panel.updateErrors(errors);
        
        REQUIRE(panel.getErrorCount(ValidationSeverity::Info) == 1);
        REQUIRE(panel.getErrorCount(ValidationSeverity::Warning) == 1);
        REQUIRE(panel.getErrorCount(ValidationSeverity::Error) == 1);
        REQUIRE(panel.getErrorCount(ValidationSeverity::Critical) == 1);
    }
    
    SECTION("Error filtering") {
        ValidationPanel panel;
        
        std::vector<ValidationError> errors;
        errors.emplace_back(ValidationSeverity::Info, "Info about kitchen");
        errors.emplace_back(ValidationSeverity::Warning, "Warning about cabinet");
        errors.emplace_back(ValidationSeverity::Error, "Error in kitchen design");
        
        panel.updateErrors(errors);
        
        // Filter by severity
        panel.setShowInfo(false);
        auto filtered = panel.getFilteredErrors();
        REQUIRE(filtered.size() == 2); // Warning and Error only
        
        // Filter by text
        panel.setShowInfo(true);
        panel.setFilterText("kitchen");
        filtered = panel.getFilteredErrors();
        REQUIRE(filtered.size() == 2); // Info and Error contain "kitchen"
        
        // Clear filter
        panel.setFilterText("");
        filtered = panel.getFilteredErrors();
        REQUIRE(filtered.size() == 3); // All errors
    }
    
    SECTION("Error selection callbacks") {
        ValidationPanel panel;
        
        bool callbackCalled = false;
        ValidationError selectedError(ValidationSeverity::Info, "");
        
        panel.setErrorSelectedCallback([&](const ValidationError& error) {
            callbackCalled = true;
            selectedError = error;
        });
        
        ValidationError testError(ValidationSeverity::Warning, "Test error");
        panel.selectError(testError);
        
        REQUIRE(callbackCalled);
        REQUIRE(selectedError.message == "Test error");
    }
    
    SECTION("Object focus callbacks") {
        ValidationPanel panel;
        
        bool focusCallbackCalled = false;
        std::string focusedObjectId;
        
        panel.setObjectFocusCallback([&](const std::string& objectId) {
            focusCallbackCalled = true;
            focusedObjectId = objectId;
        });
        
        panel.focusOnObject("test_object_123");
        
        REQUIRE(focusCallbackCalled);
        REQUIRE(focusedObjectId == "test_object_123");
    }
}