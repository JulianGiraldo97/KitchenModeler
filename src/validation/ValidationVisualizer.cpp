#include "ValidationVisualizer.h"
#include <algorithm>
#include <cmath>

namespace KitchenCAD {
namespace Validation {

ValidationVisualizer::ValidationVisualizer(IValidationRenderer* renderer)
    : renderer_(renderer), showInfo_(true), showWarnings_(true), 
      showErrors_(true), showCritical_(true), enableBlinking_(true),
      iconScale_(1.0), animationTime_(0.0) {}

void ValidationVisualizer::updateVisuals(const std::vector<ValidationError>& errors) {
    clearVisuals();
    
    for (const auto& error : errors) {
        ValidationVisual visual = createVisualForError(error);
        visuals_.push_back(visual);
    }
    
    removeDuplicates();
    updateVisibility();
}

void ValidationVisualizer::addVisual(const ValidationError& error, const Geometry::Point3D& position,
                                   ValidationVisual::VisualType type) {
    ValidationVisual visual(error, position, type);
    visuals_.push_back(visual);
    updateVisibility();
}

void ValidationVisualizer::removeVisualsForObject(const std::string& objectId) {
    visuals_.erase(
        std::remove_if(visuals_.begin(), visuals_.end(),
                      [&objectId](const ValidationVisual& visual) {
                          return visual.error.objectId == objectId;
                      }),
        visuals_.end()
    );
}

void ValidationVisualizer::clearVisuals() {
    visuals_.clear();
    if (renderer_) {
        renderer_->clearValidationVisuals();
    }
}

void ValidationVisualizer::handleVisualClicked(const ValidationVisual& visual) {
    if (visualClickedCallback_) {
        visualClickedCallback_(visual);
    }
}

ValidationVisual* ValidationVisualizer::findVisualAtPosition(const Geometry::Point3D& position, double tolerance) {
    for (auto& visual : visuals_) {
        if (visual.visible && visual.position.distanceTo(position) <= tolerance) {
            return &visual;
        }
    }
    return nullptr;
}

ValidationVisualizer::VisualStatistics ValidationVisualizer::getStatistics() const {
    VisualStatistics stats{};
    stats.totalVisuals = visuals_.size();
    
    for (const auto& visual : visuals_) {
        switch (visual.error.severity) {
            case ValidationSeverity::Info: stats.infoCount++; break;
            case ValidationSeverity::Warning: stats.warningCount++; break;
            case ValidationSeverity::Error: stats.errorCount++; break;
            case ValidationSeverity::Critical: stats.criticalCount++; break;
        }
        if (visual.visible) stats.visibleCount++;
    }
    
    return stats;
}

void ValidationVisualizer::updateAnimation(double deltaTime) {
    animationTime_ += deltaTime;
    updateBlinking(deltaTime);
}

void ValidationVisualizer::render() {
    if (!renderer_) return;
    
    for (const auto& visual : visuals_) {
        if (visual.visible && shouldShowVisual(visual)) {
            renderer_->renderValidationVisual(visual);
        }
    }
}

ValidationVisual ValidationVisualizer::createVisualForError(const ValidationError& error) {
    ValidationVisual::VisualType type = getRecommendedVisualType(error.severity);
    return ValidationVisual(error, error.location, type);
}

ValidationVisual::VisualType ValidationVisualizer::getRecommendedVisualType(ValidationSeverity severity) {
    switch (severity) {
        case ValidationSeverity::Info: return ValidationVisual::VisualType::Icon;
        case ValidationSeverity::Warning: return ValidationVisual::VisualType::Highlight;
        case ValidationSeverity::Error: return ValidationVisual::VisualType::Outline;
        case ValidationSeverity::Critical: return ValidationVisual::VisualType::Overlay;
    }
    return ValidationVisual::VisualType::Icon;
}

Geometry::BoundingBox ValidationVisualizer::createHighlightArea(const Geometry::Point3D& center, 
                                                               const Geometry::Vector3D& size) {
    Geometry::Point3D min(center.x - size.x/2, center.y - size.y/2, center.z - size.z/2);
    Geometry::Point3D max(center.x + size.x/2, center.y + size.y/2, center.z + size.z/2);
    return Geometry::BoundingBox(min, max);
}

void ValidationVisualizer::updateVisibility() {
    for (auto& visual : visuals_) {
        visual.visible = shouldShowVisual(visual);
    }
}

bool ValidationVisualizer::shouldShowVisual(const ValidationVisual& visual) const {
    switch (visual.error.severity) {
        case ValidationSeverity::Info: return showInfo_;
        case ValidationSeverity::Warning: return showWarnings_;
        case ValidationSeverity::Error: return showErrors_;
        case ValidationSeverity::Critical: return showCritical_;
    }
    return true;
}

void ValidationVisualizer::updateBlinking(double deltaTime) {
    if (!enableBlinking_) return;
    
    for (auto& visual : visuals_) {
        if (visual.blinking) {
            double phase = std::fmod(animationTime_ * visual.blinkRate, 1.0);
            visual.color.a = 0.3f + 0.7f * static_cast<float>(std::sin(phase * 2.0 * M_PI) * 0.5 + 0.5);
        }
    }
}

void ValidationVisualizer::removeDuplicates() {
    // Remove visuals with identical error messages and positions
    auto it = std::unique(visuals_.begin(), visuals_.end(),
        [](const ValidationVisual& a, const ValidationVisual& b) {
            return a.error.message == b.error.message && 
                   a.error.objectId == b.error.objectId &&
                   a.position.distanceTo(b.position) < 0.01;
        });
    visuals_.erase(it, visuals_.end());
}

// ValidationPanel Implementation

ValidationPanel::ValidationPanel()
    : showInfo_(true), showWarnings_(true), showErrors_(true), showCritical_(true) {}

void ValidationPanel::updateErrors(const std::vector<ValidationError>& errors) {
    errors_ = errors;
}

std::vector<ValidationError> ValidationPanel::getFilteredErrors() const {
    std::vector<ValidationError> filtered;
    
    for (const auto& error : errors_) {
        if (matchesFilter(error)) {
            filtered.push_back(error);
        }
    }
    
    return filtered;
}

void ValidationPanel::selectError(const ValidationError& error) {
    if (errorSelectedCallback_) {
        errorSelectedCallback_(error);
    }
}

void ValidationPanel::focusOnObject(const std::string& objectId) {
    if (objectFocusCallback_) {
        objectFocusCallback_(objectId);
    }
}

size_t ValidationPanel::getErrorCount(ValidationSeverity severity) const {
    return std::count_if(errors_.begin(), errors_.end(),
        [severity](const ValidationError& error) {
            return error.severity == severity;
        });
}

bool ValidationPanel::matchesFilter(const ValidationError& error) const {
    // Check severity filter
    bool severityMatch = false;
    switch (error.severity) {
        case ValidationSeverity::Info: severityMatch = showInfo_; break;
        case ValidationSeverity::Warning: severityMatch = showWarnings_; break;
        case ValidationSeverity::Error: severityMatch = showErrors_; break;
        case ValidationSeverity::Critical: severityMatch = showCritical_; break;
    }
    
    if (!severityMatch) return false;
    
    // Check text filter
    if (!filterText_.empty()) {
        std::string lowerMessage = error.message;
        std::string lowerFilter = filterText_;
        std::transform(lowerMessage.begin(), lowerMessage.end(), lowerMessage.begin(), ::tolower);
        std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), ::tolower);
        
        return lowerMessage.find(lowerFilter) != std::string::npos;
    }
    
    return true;
}

} // namespace Validation
} // namespace KitchenCAD