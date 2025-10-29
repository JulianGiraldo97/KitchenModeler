#pragma once

#include "../interfaces/IValidationService.h"
#include "../geometry/Point3D.h"
#include "../geometry/BoundingBox.h"
#include <vector>
#include <memory>
#include <functional>

namespace KitchenCAD {
namespace Validation {

/**
 * @brief Visual representation of a validation error
 */
struct ValidationVisual {
    ValidationError error;
    Geometry::Point3D position;
    Geometry::BoundingBox highlightArea;
    
    // Visual properties
    struct Color {
        float r, g, b, a;
        Color(float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f) 
            : r(r), g(g), b(b), a(a) {}
    } color;
    
    enum class VisualType {
        Icon,           // Warning/error icon at position
        Highlight,      // Highlight object or area
        Outline,        // Outline around object
        Annotation,     // Text annotation with arrow
        Overlay         // Semi-transparent overlay
    } type;
    
    bool visible = true;
    bool blinking = false;
    double blinkRate = 1.0; // Blinks per second
    
    ValidationVisual(const ValidationError& err, const Geometry::Point3D& pos, VisualType t = VisualType::Icon)
        : error(err), position(pos), type(t) {
        setColorBySeverity(err.severity);
    }
    
private:
    void setColorBySeverity(ValidationSeverity severity) {
        switch (severity) {
            case ValidationSeverity::Info:
                color = Color(0.2f, 0.6f, 1.0f, 0.8f); // Blue
                break;
            case ValidationSeverity::Warning:
                color = Color(1.0f, 0.8f, 0.0f, 0.9f); // Yellow/Orange
                break;
            case ValidationSeverity::Error:
                color = Color(1.0f, 0.3f, 0.0f, 0.9f); // Red-Orange
                break;
            case ValidationSeverity::Critical:
                color = Color(1.0f, 0.0f, 0.0f, 1.0f); // Bright Red
                blinking = true;
                break;
        }
    }
};

/**
 * @brief Interface for rendering validation visuals
 */
class IValidationRenderer {
public:
    virtual ~IValidationRenderer() = default;
    
    /**
     * @brief Render a validation visual
     */
    virtual void renderValidationVisual(const ValidationVisual& visual) = 0;
    
    /**
     * @brief Clear all validation visuals
     */
    virtual void clearValidationVisuals() = 0;
    
    /**
     * @brief Set visibility of validation visuals
     */
    virtual void setValidationVisualsVisible(bool visible) = 0;
};

/**
 * @brief Manages visual representation of validation errors
 * 
 * This class handles the visual feedback system for validation errors,
 * implementing requirement 14.5: mostrar advertencias visuales para violaciones de restricciones
 */
class ValidationVisualizer {
private:
    std::vector<ValidationVisual> visuals_;
    IValidationRenderer* renderer_;
    
    // Configuration
    bool showInfo_;
    bool showWarnings_;
    bool showErrors_;
    bool showCritical_;
    bool enableBlinking_;
    double iconScale_;
    
    // Callbacks
    std::function<void(const ValidationVisual&)> visualClickedCallback_;
    
public:
    /**
     * @brief Constructor
     */
    explicit ValidationVisualizer(IValidationRenderer* renderer = nullptr);
    
    /**
     * @brief Set the renderer for drawing visuals
     */
    void setRenderer(IValidationRenderer* renderer) { renderer_ = renderer; }
    
    /**
     * @brief Update visuals from validation errors
     */
    void updateVisuals(const std::vector<ValidationError>& errors);
    
    /**
     * @brief Add a single validation visual
     */
    void addVisual(const ValidationError& error, const Geometry::Point3D& position,
                   ValidationVisual::VisualType type = ValidationVisual::VisualType::Icon);
    
    /**
     * @brief Remove visuals for a specific object
     */
    void removeVisualsForObject(const std::string& objectId);
    
    /**
     * @brief Clear all visuals
     */
    void clearVisuals();
    
    /**
     * @brief Get all current visuals
     */
    const std::vector<ValidationVisual>& getVisuals() const { return visuals_; }
    
    /**
     * @brief Set visibility by severity level
     */
    void setShowInfo(bool show) { showInfo_ = show; updateVisibility(); }
    void setShowWarnings(bool show) { showWarnings_ = show; updateVisibility(); }
    void setShowErrors(bool show) { showErrors_ = show; updateVisibility(); }
    void setShowCritical(bool show) { showCritical_ = show; updateVisibility(); }
    
    /**
     * @brief Get visibility settings
     */
    bool getShowInfo() const { return showInfo_; }
    bool getShowWarnings() const { return showWarnings_; }
    bool getShowErrors() const { return showErrors_; }
    bool getShowCritical() const { return showCritical_; }
    
    /**
     * @brief Enable/disable blinking for critical errors
     */
    void setBlinkingEnabled(bool enabled) { enableBlinking_ = enabled; }
    bool isBlinkingEnabled() const { return enableBlinking_; }
    
    /**
     * @brief Set icon scale factor
     */
    void setIconScale(double scale) { iconScale_ = scale; }
    double getIconScale() const { return iconScale_; }
    
    /**
     * @brief Set callback for when a visual is clicked
     */
    void setVisualClickedCallback(std::function<void(const ValidationVisual&)> callback) {
        visualClickedCallback_ = callback;
    }
    
    /**
     * @brief Handle click on visual (call from UI)
     */
    void handleVisualClicked(const ValidationVisual& visual);
    
    /**
     * @brief Find visual at screen position
     */
    ValidationVisual* findVisualAtPosition(const Geometry::Point3D& position, double tolerance = 0.1);
    
    /**
     * @brief Get statistics about current visuals
     */
    struct VisualStatistics {
        size_t totalVisuals;
        size_t infoCount;
        size_t warningCount;
        size_t errorCount;
        size_t criticalCount;
        size_t visibleCount;
    };
    
    VisualStatistics getStatistics() const;
    
    /**
     * @brief Update animation state (call from render loop)
     */
    void updateAnimation(double deltaTime);
    
    /**
     * @brief Render all visuals
     */
    void render();
    
    /**
     * @brief Create visual for validation error with automatic positioning
     */
    static ValidationVisual createVisualForError(const ValidationError& error);
    
    /**
     * @brief Get recommended visual type for error severity
     */
    static ValidationVisual::VisualType getRecommendedVisualType(ValidationSeverity severity);
    
    /**
     * @brief Create highlight area for object
     */
    static Geometry::BoundingBox createHighlightArea(const Geometry::Point3D& center, 
                                                    const Geometry::Vector3D& size);

private:
    /**
     * @brief Update visibility of all visuals based on settings
     */
    void updateVisibility();
    
    /**
     * @brief Check if visual should be visible based on severity
     */
    bool shouldShowVisual(const ValidationVisual& visual) const;
    
    /**
     * @brief Update blinking state for visuals
     */
    void updateBlinking(double deltaTime);
    
    /**
     * @brief Remove duplicate visuals for the same error
     */
    void removeDuplicates();
    
    // Animation state
    double animationTime_;
};

/**
 * @brief Validation panel for displaying error list
 */
class ValidationPanel {
private:
    std::vector<ValidationError> errors_;
    std::function<void(const ValidationError&)> errorSelectedCallback_;
    std::function<void(const std::string&)> objectFocusCallback_;
    
    // Filtering
    bool showInfo_;
    bool showWarnings_;
    bool showErrors_;
    bool showCritical_;
    std::string filterText_;
    
public:
    ValidationPanel();
    
    /**
     * @brief Update the error list
     */
    void updateErrors(const std::vector<ValidationError>& errors);
    
    /**
     * @brief Get filtered errors for display
     */
    std::vector<ValidationError> getFilteredErrors() const;
    
    /**
     * @brief Set error selection callback
     */
    void setErrorSelectedCallback(std::function<void(const ValidationError&)> callback) {
        errorSelectedCallback_ = callback;
    }
    
    /**
     * @brief Set object focus callback
     */
    void setObjectFocusCallback(std::function<void(const std::string&)> callback) {
        objectFocusCallback_ = callback;
    }
    
    /**
     * @brief Handle error selection
     */
    void selectError(const ValidationError& error);
    
    /**
     * @brief Focus on object
     */
    void focusOnObject(const std::string& objectId);
    
    /**
     * @brief Set filter settings
     */
    void setShowInfo(bool show) { showInfo_ = show; }
    void setShowWarnings(bool show) { showWarnings_ = show; }
    void setShowErrors(bool show) { showErrors_ = show; }
    void setShowCritical(bool show) { showCritical_ = show; }
    void setFilterText(const std::string& filter) { filterText_ = filter; }
    
    /**
     * @brief Get filter settings
     */
    bool getShowInfo() const { return showInfo_; }
    bool getShowWarnings() const { return showWarnings_; }
    bool getShowErrors() const { return showErrors_; }
    bool getShowCritical() const { return showCritical_; }
    const std::string& getFilterText() const { return filterText_; }
    
    /**
     * @brief Get error count by severity
     */
    size_t getErrorCount(ValidationSeverity severity) const;
    
    /**
     * @brief Clear all errors
     */
    void clearErrors() { errors_.clear(); }

private:
    /**
     * @brief Check if error matches current filter
     */
    bool matchesFilter(const ValidationError& error) const;
};

} // namespace Validation
} // namespace KitchenCAD