#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include "../interfaces/IRenderEngine.h"
#include "../interfaces/ISceneManager.h"
#include "../models/Project.h"
#include "../core/Camera3D.h"
#include "../core/Light.h"
#include "../geometry/BoundingBox.h"
#include <unordered_map>
#include <cmath>

namespace KitchenCAD {
namespace Controllers {

using namespace Models;

/**
 * @brief Render controller for managing rendering operations
 * 
 * This controller handles all rendering-related operations including scene rendering,
 * camera management, lighting, and render settings. Implements requirements 11.1, 11.2, 11.3, 11.4, 11.5.
 */
class RenderController {
private:
    std::unique_ptr<IRenderEngine> renderEngine_;
    std::unique_ptr<ISceneManager> sceneManager_;
    Project* currentProject_;
    
    // Render state
    RenderSettings currentSettings_;
    Camera3D currentCamera_;
    std::vector<std::unique_ptr<Light>> lights_;
    
    // Predefined cameras
    std::unordered_map<std::string, Camera3D> predefinedCameras_;
    
    // Callbacks
    std::function<void()> renderCompletedCallback_;
    std::function<void(const std::string&)> viewChangedCallback_;
    std::function<void(const std::string&)> errorCallback_;

public:
    /**
     * @brief Constructor
     */
    RenderController(std::unique_ptr<IRenderEngine> renderEngine,
                    std::unique_ptr<ISceneManager> sceneManager);
    
    /**
     * @brief Destructor
     */
    ~RenderController() = default;
    
    // Initialization and lifecycle
    
    /**
     * @brief Initialize render engine
     */
    bool initialize(int width, int height);
    
    /**
     * @brief Shutdown render engine
     */
    void shutdown();
    
    /**
     * @brief Check if render engine is initialized
     */
    bool isInitialized() const;
    
    // Project management
    
    /**
     * @brief Set current project
     */
    void setCurrentProject(Project* project);
    
    /**
     * @brief Get current project
     */
    Project* getCurrentProject() const { return currentProject_; }
    
    /**
     * @brief Check if project is loaded
     */
    bool hasProject() const { return currentProject_ != nullptr; }
    
    // Scene rendering
    
    /**
     * @brief Render current scene
     */
    bool renderScene();
    
    /**
     * @brief Render scene with custom settings
     */
    bool renderScene(const RenderSettings& settings);
    
    /**
     * @brief Render scene to buffer
     */
    bool renderToBuffer(unsigned char* buffer, const RenderSettings& settings = RenderSettings());
    
    /**
     * @brief Render scene to file
     */
    bool renderToFile(const std::string& filePath, const RenderSettings& settings = RenderSettings());
    
    /**
     * @brief Render specific objects only
     */
    bool renderObjects(const std::vector<std::string>& objectIds, 
                      const RenderSettings& settings = RenderSettings());
    
    // Viewport management
    
    /**
     * @brief Set viewport dimensions
     */
    void setViewport(int x, int y, int width, int height);
    
    /**
     * @brief Get viewport dimensions
     */
    void getViewport(int& x, int& y, int& width, int& height) const;
    
    /**
     * @brief Resize viewport
     */
    void resizeViewport(int width, int height);
    
    // Camera management
    
    /**
     * @brief Set current camera
     */
    void setCamera(const Camera3D& camera);
    
    /**
     * @brief Get current camera
     */
    const Camera3D& getCamera() const { return currentCamera_; }
    
    /**
     * @brief Move camera to position
     */
    void setCameraPosition(const Point3D& position);
    
    /**
     * @brief Set camera target (look at point)
     */
    void setCameraTarget(const Point3D& target);
    
    /**
     * @brief Set camera up vector
     */
    void setCameraUp(const Vector3D& up);
    
    /**
     * @brief Set camera field of view
     */
    void setCameraFOV(double fov);
    
    /**
     * @brief Set camera near/far planes
     */
    void setCameraNearFar(double nearPlane, double farPlane);
    
    /**
     * @brief Orbit camera around target
     */
    void orbitCamera(double deltaAzimuth, double deltaElevation);
    
    /**
     * @brief Pan camera
     */
    void panCamera(double deltaX, double deltaY);
    
    /**
     * @brief Zoom camera
     */
    void zoomCamera(double factor);
    
    /**
     * @brief Frame all objects in view
     */
    void frameAll();
    
    /**
     * @brief Frame selected objects
     */
    void frameSelection();
    
    /**
     * @brief Reset camera to default position
     */
    void resetCamera();
    
    // Predefined camera views
    
    /**
     * @brief Set camera to predefined view
     */
    void setView(const std::string& viewName);
    
    /**
     * @brief Get available predefined views
     */
    std::vector<std::string> getAvailableViews() const;
    
    /**
     * @brief Add predefined camera view
     */
    void addPredefinedView(const std::string& name, const Camera3D& camera);
    
    /**
     * @brief Remove predefined view
     */
    bool removePredefinedView(const std::string& name);
    
    /**
     * @brief Set standard orthographic views
     */
    void setTopView();
    void setFrontView();
    void setRightView();
    void setIsometricView();
    
    // Lighting management
    
    /**
     * @brief Add light to scene
     */
    std::string addLight(std::unique_ptr<Light> light);
    
    /**
     * @brief Remove light from scene
     */
    bool removeLight(const std::string& lightId);
    
    /**
     * @brief Clear all lights
     */
    void clearLights();
    
    /**
     * @brief Get light IDs
     */
    std::vector<std::string> getLightIds() const;
    
    /**
     * @brief Set up default lighting
     */
    void setupDefaultLighting();
    
    /**
     * @brief Add ambient light
     */
    std::string addAmbientLight(float intensity = 0.3f, 
                               float r = 1.0f, float g = 1.0f, float b = 1.0f);
    
    /**
     * @brief Add directional light (sun)
     */
    std::string addDirectionalLight(const Vector3D& direction,
                                   float intensity = 1.0f,
                                   float r = 1.0f, float g = 1.0f, float b = 1.0f);
    
    /**
     * @brief Add point light
     */
    std::string addPointLight(const Point3D& position,
                             float intensity = 1.0f,
                             float r = 1.0f, float g = 1.0f, float b = 1.0f);
    
    /**
     * @brief Add spot light
     */
    std::string addSpotLight(const Point3D& position, const Vector3D& direction,
                            float coneAngle = 45.0f, float intensity = 1.0f,
                            float r = 1.0f, float g = 1.0f, float b = 1.0f);
    
    // Render settings
    
    /**
     * @brief Set render settings
     */
    void setRenderSettings(const RenderSettings& settings);
    
    /**
     * @brief Get current render settings
     */
    const RenderSettings& getRenderSettings() const { return currentSettings_; }
    
    /**
     * @brief Set render quality
     */
    void setRenderQuality(RenderQuality quality);
    
    /**
     * @brief Set view mode
     */
    void setViewMode(ViewMode mode);
    
    /**
     * @brief Get current view mode
     */
    ViewMode getViewMode() const;
    
    /**
     * @brief Enable/disable shadows
     */
    void setShadowsEnabled(bool enabled);
    
    /**
     * @brief Enable/disable reflections
     */
    void setReflectionsEnabled(bool enabled);
    
    /**
     * @brief Enable/disable ambient occlusion
     */
    void setAmbientOcclusionEnabled(bool enabled);
    
    /**
     * @brief Set anti-aliasing samples
     */
    void setAntiAliasingSamples(int samples);
    
    /**
     * @brief Set gamma correction
     */
    void setGamma(double gamma);
    
    // Material management
    
    /**
     * @brief Register material for rendering
     */
    void registerMaterial(const std::string& materialId, std::unique_ptr<Material> material);
    
    /**
     * @brief Unregister material
     */
    void unregisterMaterial(const std::string& materialId);
    
    /**
     * @brief Check if material is registered
     */
    bool hasMaterial(const std::string& materialId) const;
    
    // Selection and picking
    
    /**
     * @brief Pick object at screen coordinates
     */
    std::string pickObject(int screenX, int screenY);
    
    /**
     * @brief Pick objects in rectangle
     */
    std::vector<std::string> pickObjects(int x1, int y1, int x2, int y2);
    
    // Coordinate conversion
    
    /**
     * @brief Convert screen coordinates to world coordinates
     */
    Point3D screenToWorld(int screenX, int screenY, double depth = 0.0);
    
    /**
     * @brief Convert world coordinates to screen coordinates
     */
    Point3D worldToScreen(const Point3D& worldPos);
    
    // Visual aids and debugging
    
    /**
     * @brief Enable/disable wireframe mode
     */
    void setWireframeEnabled(bool enabled);
    
    /**
     * @brief Enable/disable bounding box display
     */
    void setBoundingBoxesEnabled(bool enabled);
    
    /**
     * @brief Enable/disable normal vectors display
     */
    void setNormalsEnabled(bool enabled);
    
    /**
     * @brief Set background color
     */
    void setBackgroundColor(float r, float g, float b, float a = 1.0f);
    
    /**
     * @brief Enable/disable grid display
     */
    void setGridEnabled(bool enabled);
    
    /**
     * @brief Set grid parameters
     */
    void setGridParameters(double spacing, int lineCount, 
                          float r = 0.5f, float g = 0.5f, float b = 0.5f);
    
    // Frame buffer operations
    
    /**
     * @brief Capture current frame buffer
     */
    bool captureFrameBuffer(unsigned char* buffer, RenderFormat format = RenderFormat::RGB);
    
    /**
     * @brief Save screenshot
     */
    bool saveScreenshot(const std::string& filePath, RenderFormat format = RenderFormat::RGB);
    
    // Performance and statistics
    
    /**
     * @brief Get triangle count in scene
     */
    size_t getTriangleCount() const;
    
    /**
     * @brief Get vertex count in scene
     */
    size_t getVertexCount() const;
    
    /**
     * @brief Get last render time
     */
    double getLastRenderTime() const;
    
    /**
     * @brief Get video memory usage
     */
    size_t getVideoMemoryUsage() const;
    
    /**
     * @brief Get render statistics
     */
    struct RenderStatistics {
        size_t triangleCount = 0;
        size_t vertexCount = 0;
        double lastRenderTime = 0.0;
        size_t videoMemoryUsage = 0;
        int frameRate = 0;
        size_t lightCount = 0;
        size_t materialCount = 0;
    };
    RenderStatistics getRenderStatistics() const;
    
    // Animation and time-based rendering
    
    /**
     * @brief Set animation time
     */
    void setAnimationTime(double time);
    
    /**
     * @brief Get animation time
     */
    double getAnimationTime() const;
    
    /**
     * @brief Enable/disable animation
     */
    void setAnimationEnabled(bool enabled);
    
    // Callbacks
    
    /**
     * @brief Set callback for render completed events
     */
    void setRenderCompletedCallback(std::function<void()> callback) {
        renderCompletedCallback_ = callback;
    }
    
    /**
     * @brief Set callback for view changed events
     */
    void setViewChangedCallback(std::function<void(const std::string&)> callback) {
        viewChangedCallback_ = callback;
    }
    
    /**
     * @brief Set callback for errors
     */
    void setErrorCallback(std::function<void(const std::string&)> callback) {
        errorCallback_ = callback;
    }
    
    // Utility methods
    
    /**
     * @brief Calculate optimal camera position for scene
     */
    Camera3D calculateOptimalCamera() const;
    
    /**
     * @brief Get scene bounding box
     */
    BoundingBox getSceneBounds() const;
    
    /**
     * @brief Check if point is visible in current view
     */
    bool isPointVisible(const Point3D& point) const;

private:
    double animationTime_;
    bool animationEnabled_;
    
    /**
     * @brief Initialize predefined camera views
     */
    void initializePredefinedViews();
    
    /**
     * @brief Update render engine with current scene
     */
    void updateRenderEngine();
    
    /**
     * @brief Get scene objects for rendering
     */
    std::vector<SceneObject*> getSceneObjects() const;
    
    /**
     * @brief Notify callbacks
     */
    void notifyRenderCompleted();
    void notifyViewChanged(const std::string& viewName);
    void notifyError(const std::string& error);
    
    /**
     * @brief Calculate camera parameters
     */
    void calculateCameraParameters(const BoundingBox& bounds, Camera3D& camera) const;
    
    /**
     * @brief Generate unique light ID
     */
    std::string generateLightId() const;
};

} // namespace Controllers
} // namespace KitchenCAD