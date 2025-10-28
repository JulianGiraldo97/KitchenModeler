#pragma once

#include "../geometry/Point3D.h"
#include "../geometry/Vector3D.h"
#include <string>
#include <array>

namespace KitchenCAD {

/**
 * @brief RGB color representation
 */
struct Color {
    float r, g, b, a;
    
    Color() : r(1.0f), g(1.0f), b(1.0f), a(1.0f) {}
    Color(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}
    
    // Predefined colors
    static Color white() { return Color(1.0f, 1.0f, 1.0f); }
    static Color black() { return Color(0.0f, 0.0f, 0.0f); }
    static Color red() { return Color(1.0f, 0.0f, 0.0f); }
    static Color green() { return Color(0.0f, 1.0f, 0.0f); }
    static Color blue() { return Color(0.0f, 0.0f, 1.0f); }
    
    // Convert to array for OpenGL
    std::array<float, 4> toArray() const { return {r, g, b, a}; }
    std::array<float, 3> toRGB() const { return {r, g, b}; }
};

/**
 * @brief Abstract base class for scene lights
 * 
 * This class provides the interface for all light types
 * used in scene illumination and rendering.
 */
class Light {
protected:
    std::string id_;
    bool enabled_;
    Color color_;
    float intensity_;
    
public:
    Light(const std::string& id) 
        : id_(id), enabled_(true), color_(Color::white()), intensity_(1.0f) {}
    
    virtual ~Light() = default;
    
    // Basic properties
    const std::string& getId() const { return id_; }
    
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }
    
    void setColor(const Color& color) { color_ = color; }
    const Color& getColor() const { return color_; }
    
    void setIntensity(float intensity) { intensity_ = std::max(0.0f, intensity); }
    float getIntensity() const { return intensity_; }
    
    // Virtual methods for specific light types
    virtual std::string getType() const = 0;
    virtual std::unique_ptr<Light> clone() const = 0;
};

/**
 * @brief Ambient light that illuminates all objects uniformly
 */
class AmbientLight : public Light {
public:
    AmbientLight(const std::string& id) : Light(id) {}
    
    std::string getType() const override { return "Ambient"; }
    
    std::unique_ptr<Light> clone() const override {
        auto light = std::make_unique<AmbientLight>(id_ + "_copy");
        light->setEnabled(enabled_);
        light->setColor(color_);
        light->setIntensity(intensity_);
        return light;
    }
};

/**
 * @brief Directional light with parallel rays (like sunlight)
 */
class DirectionalLight : public Light {
private:
    Geometry::Vector3D direction_;
    
public:
    DirectionalLight(const std::string& id, const Geometry::Vector3D& direction)
        : Light(id), direction_(direction.normalized()) {}
    
    void setDirection(const Geometry::Vector3D& direction) {
        direction_ = direction.normalized();
    }
    
    const Geometry::Vector3D& getDirection() const { return direction_; }
    
    std::string getType() const override { return "Directional"; }
    
    std::unique_ptr<Light> clone() const override {
        auto light = std::make_unique<DirectionalLight>(id_ + "_copy", direction_);
        light->setEnabled(enabled_);
        light->setColor(color_);
        light->setIntensity(intensity_);
        return light;
    }
};

/**
 * @brief Point light that radiates in all directions from a position
 */
class PointLight : public Light {
private:
    Geometry::Point3D position_;
    float constantAttenuation_;
    float linearAttenuation_;
    float quadraticAttenuation_;
    
public:
    PointLight(const std::string& id, const Geometry::Point3D& position)
        : Light(id), position_(position)
        , constantAttenuation_(1.0f)
        , linearAttenuation_(0.09f)
        , quadraticAttenuation_(0.032f) {}
    
    void setPosition(const Geometry::Point3D& position) { position_ = position; }
    const Geometry::Point3D& getPosition() const { return position_; }
    
    void setAttenuation(float constant, float linear, float quadratic) {
        constantAttenuation_ = std::max(0.0f, constant);
        linearAttenuation_ = std::max(0.0f, linear);
        quadraticAttenuation_ = std::max(0.0f, quadratic);
    }
    
    float getConstantAttenuation() const { return constantAttenuation_; }
    float getLinearAttenuation() const { return linearAttenuation_; }
    float getQuadraticAttenuation() const { return quadraticAttenuation_; }
    
    // Calculate attenuation at a given distance
    float calculateAttenuation(float distance) const {
        return 1.0f / (constantAttenuation_ + 
                      linearAttenuation_ * distance + 
                      quadraticAttenuation_ * distance * distance);
    }
    
    std::string getType() const override { return "Point"; }
    
    std::unique_ptr<Light> clone() const override {
        auto light = std::make_unique<PointLight>(id_ + "_copy", position_);
        light->setEnabled(enabled_);
        light->setColor(color_);
        light->setIntensity(intensity_);
        light->setAttenuation(constantAttenuation_, linearAttenuation_, quadraticAttenuation_);
        return light;
    }
};

/**
 * @brief Spot light that illuminates a cone-shaped area
 */
class SpotLight : public Light {
private:
    Geometry::Point3D position_;
    Geometry::Vector3D direction_;
    float innerConeAngle_;  // In radians
    float outerConeAngle_;  // In radians
    float constantAttenuation_;
    float linearAttenuation_;
    float quadraticAttenuation_;
    
public:
    SpotLight(const std::string& id, 
              const Geometry::Point3D& position,
              const Geometry::Vector3D& direction,
              float innerAngle = 0.5f,
              float outerAngle = 0.8f)
        : Light(id), position_(position), direction_(direction.normalized())
        , innerConeAngle_(innerAngle), outerConeAngle_(outerAngle)
        , constantAttenuation_(1.0f)
        , linearAttenuation_(0.09f)
        , quadraticAttenuation_(0.032f) {}
    
    void setPosition(const Geometry::Point3D& position) { position_ = position; }
    const Geometry::Point3D& getPosition() const { return position_; }
    
    void setDirection(const Geometry::Vector3D& direction) {
        direction_ = direction.normalized();
    }
    const Geometry::Vector3D& getDirection() const { return direction_; }
    
    void setConeAngles(float innerAngle, float outerAngle) {
        innerConeAngle_ = std::max(0.0f, std::min(innerAngle, outerAngle));
        outerConeAngle_ = std::max(innerConeAngle_, outerAngle);
    }
    
    float getInnerConeAngle() const { return innerConeAngle_; }
    float getOuterConeAngle() const { return outerConeAngle_; }
    
    void setAttenuation(float constant, float linear, float quadratic) {
        constantAttenuation_ = std::max(0.0f, constant);
        linearAttenuation_ = std::max(0.0f, linear);
        quadraticAttenuation_ = std::max(0.0f, quadratic);
    }
    
    float getConstantAttenuation() const { return constantAttenuation_; }
    float getLinearAttenuation() const { return linearAttenuation_; }
    float getQuadraticAttenuation() const { return quadraticAttenuation_; }
    
    // Calculate spot factor for a given direction from the light
    float calculateSpotFactor(const Geometry::Vector3D& toPoint) const {
        Geometry::Vector3D lightDir = toPoint.normalized();
        float cosAngle = direction_.dot(lightDir);
        float cosInner = std::cos(innerConeAngle_);
        float cosOuter = std::cos(outerConeAngle_);
        
        if (cosAngle > cosInner) {
            return 1.0f;  // Inside inner cone
        } else if (cosAngle > cosOuter) {
            // Smooth falloff between inner and outer cone
            return (cosAngle - cosOuter) / (cosInner - cosOuter);
        } else {
            return 0.0f;  // Outside outer cone
        }
    }
    
    std::string getType() const override { return "Spot"; }
    
    std::unique_ptr<Light> clone() const override {
        auto light = std::make_unique<SpotLight>(id_ + "_copy", position_, direction_, 
                                               innerConeAngle_, outerConeAngle_);
        light->setEnabled(enabled_);
        light->setColor(color_);
        light->setIntensity(intensity_);
        light->setAttenuation(constantAttenuation_, linearAttenuation_, quadraticAttenuation_);
        return light;
    }
};

} // namespace KitchenCAD