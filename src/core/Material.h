#pragma once

#include "Light.h"  // For Color struct
#include <string>
#include <memory>

namespace KitchenCAD {

/**
 * @brief Material properties for physically-based rendering
 * 
 * This class represents material properties used for realistic
 * rendering including textures, colors, and physical properties.
 */
class Material {
private:
    std::string id_;
    std::string name_;
    
    // Basic color properties
    Color diffuseColor_;
    Color specularColor_;
    Color emissiveColor_;
    
    // PBR properties
    float roughness_;       // 0.0 = mirror, 1.0 = completely rough
    float metallic_;        // 0.0 = dielectric, 1.0 = metallic
    float reflectance_;     // Base reflectance for dielectrics (typically 0.04)
    float transparency_;    // 0.0 = opaque, 1.0 = fully transparent
    float indexOfRefraction_;  // IOR for transparent materials
    
    // Texture paths
    std::string diffuseTexture_;
    std::string normalTexture_;
    std::string roughnessTexture_;
    std::string metallicTexture_;
    std::string emissiveTexture_;
    std::string opacityTexture_;
    
    // Texture scaling
    float textureScaleU_;
    float textureScaleV_;
    
    // Additional properties
    bool doubleSided_;
    bool castsShadows_;
    bool receivesShadows_;
    
public:
    Material(const std::string& id, const std::string& name = "")
        : id_(id)
        , name_(name.empty() ? id : name)
        , diffuseColor_(Color::white())
        , specularColor_(Color::white())
        , emissiveColor_(Color::black())
        , roughness_(0.5f)
        , metallic_(0.0f)
        , reflectance_(0.04f)
        , transparency_(0.0f)
        , indexOfRefraction_(1.5f)
        , textureScaleU_(1.0f)
        , textureScaleV_(1.0f)
        , doubleSided_(false)
        , castsShadows_(true)
        , receivesShadows_(true)
    {}
    
    // Basic properties
    const std::string& getId() const { return id_; }
    
    void setName(const std::string& name) { name_ = name; }
    const std::string& getName() const { return name_; }
    
    // Color properties
    void setDiffuseColor(const Color& color) { diffuseColor_ = color; }
    const Color& getDiffuseColor() const { return diffuseColor_; }
    
    void setSpecularColor(const Color& color) { specularColor_ = color; }
    const Color& getSpecularColor() const { return specularColor_; }
    
    void setEmissiveColor(const Color& color) { emissiveColor_ = color; }
    const Color& getEmissiveColor() const { return emissiveColor_; }
    
    // PBR properties
    void setRoughness(float roughness) { 
        roughness_ = std::max(0.0f, std::min(1.0f, roughness)); 
    }
    float getRoughness() const { return roughness_; }
    
    void setMetallic(float metallic) { 
        metallic_ = std::max(0.0f, std::min(1.0f, metallic)); 
    }
    float getMetallic() const { return metallic_; }
    
    void setReflectance(float reflectance) { 
        reflectance_ = std::max(0.0f, std::min(1.0f, reflectance)); 
    }
    float getReflectance() const { return reflectance_; }
    
    void setTransparency(float transparency) { 
        transparency_ = std::max(0.0f, std::min(1.0f, transparency)); 
    }
    float getTransparency() const { return transparency_; }
    
    void setIndexOfRefraction(float ior) { 
        indexOfRefraction_ = std::max(1.0f, ior); 
    }
    float getIndexOfRefraction() const { return indexOfRefraction_; }
    
    // Texture properties
    void setDiffuseTexture(const std::string& path) { diffuseTexture_ = path; }
    const std::string& getDiffuseTexture() const { return diffuseTexture_; }
    
    void setNormalTexture(const std::string& path) { normalTexture_ = path; }
    const std::string& getNormalTexture() const { return normalTexture_; }
    
    void setRoughnessTexture(const std::string& path) { roughnessTexture_ = path; }
    const std::string& getRoughnessTexture() const { return roughnessTexture_; }
    
    void setMetallicTexture(const std::string& path) { metallicTexture_ = path; }
    const std::string& getMetallicTexture() const { return metallicTexture_; }
    
    void setEmissiveTexture(const std::string& path) { emissiveTexture_ = path; }
    const std::string& getEmissiveTexture() const { return emissiveTexture_; }
    
    void setOpacityTexture(const std::string& path) { opacityTexture_ = path; }
    const std::string& getOpacityTexture() const { return opacityTexture_; }
    
    // Texture scaling
    void setTextureScale(float scaleU, float scaleV) {
        textureScaleU_ = std::max(0.001f, scaleU);
        textureScaleV_ = std::max(0.001f, scaleV);
    }
    
    float getTextureScaleU() const { return textureScaleU_; }
    float getTextureScaleV() const { return textureScaleV_; }
    
    // Additional properties
    void setDoubleSided(bool doubleSided) { doubleSided_ = doubleSided; }
    bool isDoubleSided() const { return doubleSided_; }
    
    void setCastsShadows(bool casts) { castsShadows_ = casts; }
    bool castsShadows() const { return castsShadows_; }
    
    void setReceivesShadows(bool receives) { receivesShadows_ = receives; }
    bool receivesShadows() const { return receivesShadows_; }
    
    // Utility methods
    bool hasTextures() const {
        return !diffuseTexture_.empty() || !normalTexture_.empty() || 
               !roughnessTexture_.empty() || !metallicTexture_.empty() ||
               !emissiveTexture_.empty() || !opacityTexture_.empty();
    }
    
    bool isTransparent() const {
        return transparency_ > 0.0f;
    }
    
    bool isEmissive() const {
        return emissiveColor_.r > 0.0f || emissiveColor_.g > 0.0f || 
               emissiveColor_.b > 0.0f || !emissiveTexture_.empty();
    }
    
    // Cloning
    std::unique_ptr<Material> clone() const {
        auto material = std::make_unique<Material>(id_ + "_copy", name_ + "_copy");
        
        material->diffuseColor_ = diffuseColor_;
        material->specularColor_ = specularColor_;
        material->emissiveColor_ = emissiveColor_;
        
        material->roughness_ = roughness_;
        material->metallic_ = metallic_;
        material->reflectance_ = reflectance_;
        material->transparency_ = transparency_;
        material->indexOfRefraction_ = indexOfRefraction_;
        
        material->diffuseTexture_ = diffuseTexture_;
        material->normalTexture_ = normalTexture_;
        material->roughnessTexture_ = roughnessTexture_;
        material->metallicTexture_ = metallicTexture_;
        material->emissiveTexture_ = emissiveTexture_;
        material->opacityTexture_ = opacityTexture_;
        
        material->textureScaleU_ = textureScaleU_;
        material->textureScaleV_ = textureScaleV_;
        
        material->doubleSided_ = doubleSided_;
        material->castsShadows_ = castsShadows_;
        material->receivesShadows_ = receivesShadows_;
        
        return material;
    }
    
    // Predefined materials
    static std::unique_ptr<Material> createWood(const std::string& id) {
        auto material = std::make_unique<Material>(id, "Wood");
        material->setDiffuseColor(Color(0.6f, 0.4f, 0.2f));
        material->setRoughness(0.8f);
        material->setMetallic(0.0f);
        return material;
    }
    
    static std::unique_ptr<Material> createMetal(const std::string& id) {
        auto material = std::make_unique<Material>(id, "Metal");
        material->setDiffuseColor(Color(0.7f, 0.7f, 0.7f));
        material->setRoughness(0.2f);
        material->setMetallic(1.0f);
        return material;
    }
    
    static std::unique_ptr<Material> createGlass(const std::string& id) {
        auto material = std::make_unique<Material>(id, "Glass");
        material->setDiffuseColor(Color(1.0f, 1.0f, 1.0f));
        material->setRoughness(0.0f);
        material->setMetallic(0.0f);
        material->setTransparency(0.9f);
        material->setIndexOfRefraction(1.5f);
        return material;
    }
    
    static std::unique_ptr<Material> createPlastic(const std::string& id) {
        auto material = std::make_unique<Material>(id, "Plastic");
        material->setDiffuseColor(Color(0.8f, 0.8f, 0.8f));
        material->setRoughness(0.4f);
        material->setMetallic(0.0f);
        material->setReflectance(0.04f);
        return material;
    }
};

} // namespace KitchenCAD