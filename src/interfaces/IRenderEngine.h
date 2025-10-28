#pragma once

#include "../geometry/Point3D.h"
#include "../geometry/Vector3D.h"
#include <memory>
#include <vector>
#include <string>

namespace KitchenCAD {

// Forward declarations
class SceneObject;
class Camera3D;
class Light;
class Material;

/**
 * @brief Rendering quality levels
 */
enum class RenderQuality {
    Draft,      // Fast preview rendering
    Standard,   // Balanced quality/performance
    High,       // High quality rendering
    Production  // Maximum quality for final output
};

/**
 * @brief Rendering output formats
 */
enum class RenderFormat {
    RGB,
    RGBA,
    HDR
};

/**
 * @brief View modes for rendering
 */
enum class ViewMode {
    Wireframe,
    Shaded,
    ShadedWithEdges,
    Realistic,
    Raytraced
};

/**
 * @brief Light types for scene illumination
 */
enum class LightType {
    Ambient,
    Directional,
    Point,
    Spot
};

/**
 * @brief Render settings configuration
 */
struct RenderSettings {
    RenderQuality quality = RenderQuality::Standard;
    ViewMode viewMode = ViewMode::Shaded;
    int width = 1920;
    int height = 1080;
    int samples = 1;  // Anti-aliasing samples
    bool enableShadows = true;
    bool enableReflections = false;
    bool enableAmbientOcclusion = false;
    double gamma = 2.2;
    
    RenderSettings() = default;
};

/**
 * @brief Interface for 3D rendering operations
 * 
 * This interface defines the contract for rendering 3D scenes,
 * managing cameras, lights, and materials for visualization.
 */
class IRenderEngine {
public:
    virtual ~IRenderEngine() = default;
    
    // Rendering operations
    virtual bool initialize(int width, int height) = 0;
    virtual void shutdown() = 0;
    virtual bool isInitialized() const = 0;
    
    // Scene rendering
    virtual bool renderScene(const std::vector<SceneObject*>& objects, 
                           const Camera3D& camera,
                           const RenderSettings& settings = RenderSettings()) = 0;
    
    virtual bool renderToBuffer(const std::vector<SceneObject*>& objects,
                              const Camera3D& camera,
                              unsigned char* buffer,
                              const RenderSettings& settings = RenderSettings()) = 0;
    
    virtual bool renderToFile(const std::vector<SceneObject*>& objects,
                            const Camera3D& camera,
                            const std::string& filePath,
                            const RenderSettings& settings = RenderSettings()) = 0;
    
    // Viewport management
    virtual void setViewport(int x, int y, int width, int height) = 0;
    virtual void getViewport(int& x, int& y, int& width, int& height) const = 0;
    virtual void resizeViewport(int width, int height) = 0;
    
    // Camera management
    virtual void setCamera(const Camera3D& camera) = 0;
    virtual const Camera3D& getCamera() const = 0;
    
    // Lighting management
    virtual void addLight(std::unique_ptr<Light> light) = 0;
    virtual void removeLight(const std::string& lightId) = 0;
    virtual void clearLights() = 0;
    virtual std::vector<std::string> getLightIds() const = 0;
    
    // Material management
    virtual void registerMaterial(const std::string& materialId, std::unique_ptr<Material> material) = 0;
    virtual void unregisterMaterial(const std::string& materialId) = 0;
    virtual bool hasMaterial(const std::string& materialId) const = 0;
    
    // Render settings
    virtual void setRenderSettings(const RenderSettings& settings) = 0;
    virtual const RenderSettings& getRenderSettings() const = 0;
    
    // View modes
    virtual void setViewMode(ViewMode mode) = 0;
    virtual ViewMode getViewMode() const = 0;
    
    // Selection and picking
    virtual std::string pickObject(int screenX, int screenY) = 0;  // Returns object ID
    virtual std::vector<std::string> pickObjects(int x1, int y1, int x2, int y2) = 0;  // Rectangle selection
    
    // Utility functions
    virtual Geometry::Point3D screenToWorld(int screenX, int screenY, double depth = 0.0) = 0;
    virtual Geometry::Point3D worldToScreen(const Geometry::Point3D& worldPos) = 0;
    
    // Performance and debugging
    virtual void enableWireframe(bool enable) = 0;
    virtual void enableBoundingBoxes(bool enable) = 0;
    virtual void enableNormals(bool enable) = 0;
    virtual void setBackgroundColor(float r, float g, float b, float a = 1.0f) = 0;
    
    // Frame buffer operations
    virtual bool captureFrameBuffer(unsigned char* buffer, RenderFormat format = RenderFormat::RGB) = 0;
    virtual bool saveScreenshot(const std::string& filePath, RenderFormat format = RenderFormat::RGB) = 0;
    
    // Statistics
    virtual size_t getTriangleCount() const = 0;
    virtual size_t getVertexCount() const = 0;
    virtual double getLastRenderTime() const = 0;  // In milliseconds
    virtual size_t getVideoMemoryUsage() const = 0;  // In bytes
};

} // namespace KitchenCAD