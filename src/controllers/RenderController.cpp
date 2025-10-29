#include "RenderController.h"
#include "../utils/Logger.h"
#include <cmath>
#include <algorithm>
#include <set>

namespace KitchenCAD {
namespace Controllers {

RenderController::RenderController(std::unique_ptr<IRenderEngine> renderEngine,
                                 std::unique_ptr<ISceneManager> sceneManager)
    : renderEngine_(std::move(renderEngine))
    , sceneManager_(std::move(sceneManager))
    , currentProject_(nullptr)
    , animationTime_(0.0)
    , animationEnabled_(false)
{
    // Initialize default render settings
    currentSettings_.quality = RenderQuality::Standard;
    currentSettings_.viewMode = ViewMode::Shaded;
    currentSettings_.width = 1920;
    currentSettings_.height = 1080;
    currentSettings_.enableShadows = true;
    currentSettings_.enableReflections = false;
    currentSettings_.enableAmbientOcclusion = false;
    
    // Initialize default camera
    currentCamera_.setPosition(Point3D(5.0, 5.0, 5.0));
    currentCamera_.setTarget(Point3D(0.0, 0.0, 0.0));
    currentCamera_.setUp(Vector3D(0.0, 0.0, 1.0));
    currentCamera_.setFOV(45.0);
    currentCamera_.setNearFar(0.1, 1000.0);
    
    initializePredefinedViews();
}

bool RenderController::initialize(int width, int height) {
    if (!renderEngine_) {
        notifyError("Render engine not available");
        return false;
    }
    
    bool success = renderEngine_->initialize(width, height);
    
    if (success) {
        currentSettings_.width = width;
        currentSettings_.height = height;
        
        // Set up default lighting and camera
        setupDefaultLighting();
        renderEngine_->setCamera(currentCamera_);
        renderEngine_->setRenderSettings(currentSettings_);
    } else {
        notifyError("Failed to initialize render engine");
    }
    
    return success;
}

void RenderController::shutdown() {
    if (renderEngine_) {
        renderEngine_->shutdown();
    }
}

bool RenderController::isInitialized() const {
    return renderEngine_ && renderEngine_->isInitialized();
}

void RenderController::setCurrentProject(Project* project) {
    currentProject_ = project;
    
    if (sceneManager_) {
        sceneManager_->clear();
        
        // Load project objects into scene manager
        if (project) {
            for (const auto& object : project->getObjects()) {
                // Create a copy for the scene manager
                auto objectCopy = std::make_unique<SceneObject>(*object);
                sceneManager_->addObject(std::move(objectCopy));
            }
        }
    }
    
    // Frame the scene if we have objects
    if (project && !project->getObjects().empty()) {
        frameAll();
    }
}

bool RenderController::renderScene() {
    return renderScene(currentSettings_);
}

bool RenderController::renderScene(const RenderSettings& settings) {
    if (!renderEngine_ || !isInitialized()) {
        notifyError("Render engine not initialized");
        return false;
    }
    
    try {
        auto objects = getSceneObjects();
        bool success = renderEngine_->renderScene(objects, currentCamera_, settings);
        
        if (success) {
            notifyRenderCompleted();
        } else {
            notifyError("Scene rendering failed");
        }
        
        return success;
        
    } catch (const std::exception& e) {
        notifyError("Render error: " + std::string(e.what()));
        return false;
    }
}

bool RenderController::renderToBuffer(unsigned char* buffer, const RenderSettings& settings) {
    if (!renderEngine_ || !isInitialized() || !buffer) {
        notifyError("Invalid render parameters");
        return false;
    }
    
    try {
        auto objects = getSceneObjects();
        bool success = renderEngine_->renderToBuffer(objects, currentCamera_, buffer, settings);
        
        if (!success) {
            notifyError("Render to buffer failed");
        }
        
        return success;
        
    } catch (const std::exception& e) {
        notifyError("Render to buffer error: " + std::string(e.what()));
        return false;
    }
}

bool RenderController::renderToFile(const std::string& filePath, const RenderSettings& settings) {
    if (!renderEngine_ || !isInitialized()) {
        notifyError("Render engine not initialized");
        return false;
    }
    
    if (filePath.empty()) {
        notifyError("File path cannot be empty");
        return false;
    }
    
    try {
        auto objects = getSceneObjects();
        bool success = renderEngine_->renderToFile(objects, currentCamera_, filePath, settings);
        
        if (!success) {
            notifyError("Render to file failed: " + filePath);
        }
        
        return success;
        
    } catch (const std::exception& e) {
        notifyError("Render to file error: " + std::string(e.what()));
        return false;
    }
}

bool RenderController::renderObjects(const std::vector<std::string>& objectIds, 
                                   const RenderSettings& settings) {
    if (!renderEngine_ || !isInitialized()) {
        notifyError("Render engine not initialized");
        return false;
    }
    
    if (!sceneManager_) {
        notifyError("Scene manager not available");
        return false;
    }
    
    try {
        std::vector<SceneObject*> objects;
        
        for (const auto& objectId : objectIds) {
            SceneObject* object = sceneManager_->getObject(objectId);
            if (object) {
                objects.push_back(object);
            }
        }
        
        if (objects.empty()) {
            notifyError("No valid objects found for rendering");
            return false;
        }
        
        bool success = renderEngine_->renderScene(objects, currentCamera_, settings);
        
        if (success) {
            notifyRenderCompleted();
        } else {
            notifyError("Object rendering failed");
        }
        
        return success;
        
    } catch (const std::exception& e) {
        notifyError("Render objects error: " + std::string(e.what()));
        return false;
    }
}

void RenderController::setViewport(int x, int y, int width, int height) {
    if (renderEngine_) {
        renderEngine_->setViewport(x, y, width, height);
        currentSettings_.width = width;
        currentSettings_.height = height;
    }
}

void RenderController::getViewport(int& x, int& y, int& width, int& height) const {
    if (renderEngine_) {
        renderEngine_->getViewport(x, y, width, height);
    } else {
        x = y = 0;
        width = currentSettings_.width;
        height = currentSettings_.height;
    }
}

void RenderController::resizeViewport(int width, int height) {
    if (renderEngine_) {
        renderEngine_->resizeViewport(width, height);
        currentSettings_.width = width;
        currentSettings_.height = height;
        
        // Update camera aspect ratio
        currentCamera_.setAspectRatio(static_cast<double>(width) / height);
    }
}

void RenderController::setCamera(const Camera3D& camera) {
    currentCamera_ = camera;
    
    if (renderEngine_) {
        renderEngine_->setCamera(currentCamera_);
    }
    
    notifyViewChanged("Custom");
}

void RenderController::setCameraPosition(const Point3D& position) {
    currentCamera_.setPosition(position);
    
    if (renderEngine_) {
        renderEngine_->setCamera(currentCamera_);
    }
    
    notifyViewChanged("Custom");
}

void RenderController::setCameraTarget(const Point3D& target) {
    currentCamera_.setTarget(target);
    
    if (renderEngine_) {
        renderEngine_->setCamera(currentCamera_);
    }
    
    notifyViewChanged("Custom");
}

void RenderController::setCameraUp(const Vector3D& up) {
    currentCamera_.setUp(up);
    
    if (renderEngine_) {
        renderEngine_->setCamera(currentCamera_);
    }
    
    notifyViewChanged("Custom");
}

void RenderController::setCameraFOV(double fov) {
    currentCamera_.setFOV(fov);
    
    if (renderEngine_) {
        renderEngine_->setCamera(currentCamera_);
    }
}

void RenderController::setCameraNearFar(double nearPlane, double farPlane) {
    currentCamera_.setNearFar(nearPlane, farPlane);
    
    if (renderEngine_) {
        renderEngine_->setCamera(currentCamera_);
    }
}

void RenderController::orbitCamera(double deltaAzimuth, double deltaElevation) {
    Point3D position = currentCamera_.getPosition();
    Point3D target = currentCamera_.getTarget();
    
    // Calculate current spherical coordinates
    Vector3D toCamera = position - target;
    double radius = toCamera.length();
    
    if (radius < 0.001) {
        return; // Camera too close to target
    }
    
    double azimuth = std::atan2(toCamera.y, toCamera.x);
    double elevation = std::asin(toCamera.z / radius);
    
    // Apply deltas
    azimuth += deltaAzimuth;
    elevation += deltaElevation;
    
    // Clamp elevation to avoid gimbal lock
    elevation = std::max(-M_PI/2 + 0.01, std::min(M_PI/2 - 0.01, elevation));
    
    // Calculate new position
    Point3D newPosition;
    newPosition.x = target.x + radius * std::cos(elevation) * std::cos(azimuth);
    newPosition.y = target.y + radius * std::cos(elevation) * std::sin(azimuth);
    newPosition.z = target.z + radius * std::sin(elevation);
    
    setCameraPosition(newPosition);
}

void RenderController::panCamera(double deltaX, double deltaY) {
    Point3D position = currentCamera_.getPosition();
    Point3D target = currentCamera_.getTarget();
    Vector3D up = currentCamera_.getUp();
    
    // Calculate camera coordinate system
    Vector3D forward = (target - position).normalized();
    Vector3D right = forward.cross(up).normalized();
    Vector3D cameraUp = right.cross(forward).normalized();
    
    // Calculate pan distance based on distance to target
    double distance = (target - position).length();
    double panScale = distance * 0.001; // Adjust as needed
    
    // Apply pan
    Vector3D panOffset = right * (deltaX * panScale) + cameraUp * (deltaY * panScale);
    
    setCameraPosition(position + panOffset);
    setCameraTarget(target + panOffset);
}

void RenderController::zoomCamera(double factor) {
    Point3D position = currentCamera_.getPosition();
    Point3D target = currentCamera_.getTarget();
    
    Vector3D toCamera = position - target;
    double distance = toCamera.length();
    
    // Apply zoom factor
    double newDistance = distance * factor;
    
    // Clamp to reasonable limits
    newDistance = std::max(0.1, std::min(1000.0, newDistance));
    
    // Calculate new position
    Vector3D direction = toCamera.normalized();
    Point3D newPosition = target + direction * newDistance;
    
    setCameraPosition(newPosition);
}

void RenderController::frameAll() {
    BoundingBox bounds = getSceneBounds();
    
    if (bounds.isEmpty()) {
        resetCamera();
        return;
    }
    
    Camera3D optimalCamera = calculateOptimalCamera();
    setCamera(optimalCamera);
    
    notifyViewChanged("Frame All");
}

void RenderController::frameSelection() {
    if (!sceneManager_) {
        frameAll();
        return;
    }
    
    auto selection = sceneManager_->getSelection();
    if (selection.empty()) {
        frameAll();
        return;
    }
    
    // Calculate bounding box of selected objects
    BoundingBox bounds;
    bool first = true;
    
    for (const auto& objectId : selection) {
        const SceneObject* object = sceneManager_->getObject(objectId);
        if (object) {
            // Calculate object bounds (simplified)
            Transform3D transform = object->getTransform();
            Point3D pos = transform.translation;
            
            if (first) {
                bounds = BoundingBox(pos, pos);
                first = false;
            } else {
                bounds.expandToInclude(pos);
            }
        }
    }
    
    if (!bounds.isEmpty()) {
        Camera3D camera = currentCamera_;
        calculateCameraParameters(bounds, camera);
        setCamera(camera);
        
        notifyViewChanged("Frame Selection");
    }
}

void RenderController::resetCamera() {
    currentCamera_.setPosition(Point3D(5.0, 5.0, 5.0));
    currentCamera_.setTarget(Point3D(0.0, 0.0, 0.0));
    currentCamera_.setUp(Vector3D(0.0, 0.0, 1.0));
    currentCamera_.setFOV(45.0);
    
    if (renderEngine_) {
        renderEngine_->setCamera(currentCamera_);
    }
    
    notifyViewChanged("Default");
}

void RenderController::setView(const std::string& viewName) {
    auto it = predefinedCameras_.find(viewName);
    if (it != predefinedCameras_.end()) {
        setCamera(it->second);
        notifyViewChanged(viewName);
    } else {
        notifyError("Unknown view: " + viewName);
    }
}

std::vector<std::string> RenderController::getAvailableViews() const {
    std::vector<std::string> views;
    
    for (const auto& pair : predefinedCameras_) {
        views.push_back(pair.first);
    }
    
    return views;
}

void RenderController::addPredefinedView(const std::string& name, const Camera3D& camera) {
    predefinedCameras_[name] = camera;
}

bool RenderController::removePredefinedView(const std::string& name) {
    return predefinedCameras_.erase(name) > 0;
}

void RenderController::setTopView() {
    setView("Top");
}

void RenderController::setFrontView() {
    setView("Front");
}

void RenderController::setRightView() {
    setView("Right");
}

void RenderController::setIsometricView() {
    setView("Isometric");
}

std::string RenderController::addLight(std::unique_ptr<Light> light) {
    if (!light || !renderEngine_) {
        return "";
    }
    
    std::string lightId = generateLightId();
    renderEngine_->addLight(std::move(light));
    
    return lightId;
}

bool RenderController::removeLight(const std::string& lightId) {
    if (renderEngine_) {
        renderEngine_->removeLight(lightId);
        return true;
    }
    
    return false;
}

void RenderController::clearLights() {
    if (renderEngine_) {
        renderEngine_->clearLights();
    }
}

std::vector<std::string> RenderController::getLightIds() const {
    if (renderEngine_) {
        return renderEngine_->getLightIds();
    }
    
    return {};
}

void RenderController::setupDefaultLighting() {
    if (!renderEngine_) {
        return;
    }
    
    clearLights();
    
    // Add ambient light
    addAmbientLight(0.3f);
    
    // Add main directional light (sun)
    addDirectionalLight(Vector3D(-1.0, -1.0, -1.0).normalized(), 0.8f);
    
    // Add fill light
    addDirectionalLight(Vector3D(1.0, 0.5, 0.5).normalized(), 0.3f);
}

std::string RenderController::addAmbientLight(float intensity, float r, float g, float b) {
    // This would create an ambient light object
    // For now, return a placeholder implementation
    return generateLightId();
}

std::string RenderController::addDirectionalLight(const Vector3D& direction,
                                                 float intensity,
                                                 float r, float g, float b) {
    // This would create a directional light object
    // For now, return a placeholder implementation
    return generateLightId();
}

std::string RenderController::addPointLight(const Point3D& position,
                                           float intensity,
                                           float r, float g, float b) {
    // This would create a point light object
    // For now, return a placeholder implementation
    return generateLightId();
}

std::string RenderController::addSpotLight(const Point3D& position, const Vector3D& direction,
                                          float coneAngle, float intensity,
                                          float r, float g, float b) {
    // This would create a spot light object
    // For now, return a placeholder implementation
    return generateLightId();
}

void RenderController::setRenderSettings(const RenderSettings& settings) {
    currentSettings_ = settings;
    
    if (renderEngine_) {
        renderEngine_->setRenderSettings(currentSettings_);
    }
}

void RenderController::setRenderQuality(RenderQuality quality) {
    currentSettings_.quality = quality;
    
    if (renderEngine_) {
        renderEngine_->setRenderSettings(currentSettings_);
    }
}

void RenderController::setViewMode(ViewMode mode) {
    currentSettings_.viewMode = mode;
    
    if (renderEngine_) {
        renderEngine_->setViewMode(mode);
    }
}

ViewMode RenderController::getViewMode() const {
    if (renderEngine_) {
        return renderEngine_->getViewMode();
    }
    
    return currentSettings_.viewMode;
}

void RenderController::setShadowsEnabled(bool enabled) {
    currentSettings_.enableShadows = enabled;
    
    if (renderEngine_) {
        renderEngine_->setRenderSettings(currentSettings_);
    }
}

void RenderController::setReflectionsEnabled(bool enabled) {
    currentSettings_.enableReflections = enabled;
    
    if (renderEngine_) {
        renderEngine_->setRenderSettings(currentSettings_);
    }
}

void RenderController::setAmbientOcclusionEnabled(bool enabled) {
    currentSettings_.enableAmbientOcclusion = enabled;
    
    if (renderEngine_) {
        renderEngine_->setRenderSettings(currentSettings_);
    }
}

void RenderController::setAntiAliasingSamples(int samples) {
    currentSettings_.samples = samples;
    
    if (renderEngine_) {
        renderEngine_->setRenderSettings(currentSettings_);
    }
}

void RenderController::setGamma(double gamma) {
    currentSettings_.gamma = gamma;
    
    if (renderEngine_) {
        renderEngine_->setRenderSettings(currentSettings_);
    }
}

void RenderController::registerMaterial(const std::string& materialId, std::unique_ptr<Material> material) {
    if (renderEngine_) {
        renderEngine_->registerMaterial(materialId, std::move(material));
    }
}

void RenderController::unregisterMaterial(const std::string& materialId) {
    if (renderEngine_) {
        renderEngine_->unregisterMaterial(materialId);
    }
}

bool RenderController::hasMaterial(const std::string& materialId) const {
    if (renderEngine_) {
        return renderEngine_->hasMaterial(materialId);
    }
    
    return false;
}

std::string RenderController::pickObject(int screenX, int screenY) {
    if (renderEngine_) {
        return renderEngine_->pickObject(screenX, screenY);
    }
    
    return "";
}

std::vector<std::string> RenderController::pickObjects(int x1, int y1, int x2, int y2) {
    if (renderEngine_) {
        return renderEngine_->pickObjects(x1, y1, x2, y2);
    }
    
    return {};
}

Point3D RenderController::screenToWorld(int screenX, int screenY, double depth) {
    if (renderEngine_) {
        return renderEngine_->screenToWorld(screenX, screenY, depth);
    }
    
    return Point3D();
}

Point3D RenderController::worldToScreen(const Point3D& worldPos) {
    if (renderEngine_) {
        return renderEngine_->worldToScreen(worldPos);
    }
    
    return Point3D();
}

void RenderController::setWireframeEnabled(bool enabled) {
    if (renderEngine_) {
        renderEngine_->enableWireframe(enabled);
    }
}

void RenderController::setBoundingBoxesEnabled(bool enabled) {
    if (renderEngine_) {
        renderEngine_->enableBoundingBoxes(enabled);
    }
}

void RenderController::setNormalsEnabled(bool enabled) {
    if (renderEngine_) {
        renderEngine_->enableNormals(enabled);
    }
}

void RenderController::setBackgroundColor(float r, float g, float b, float a) {
    if (renderEngine_) {
        renderEngine_->setBackgroundColor(r, g, b, a);
    }
}

void RenderController::setGridEnabled(bool enabled) {
    // This would enable/disable grid rendering
    // Implementation depends on render engine capabilities
}

void RenderController::setGridParameters(double spacing, int lineCount, 
                                        float r, float g, float b) {
    // This would set grid rendering parameters
    // Implementation depends on render engine capabilities
}

bool RenderController::captureFrameBuffer(unsigned char* buffer, RenderFormat format) {
    if (renderEngine_) {
        return renderEngine_->captureFrameBuffer(buffer, format);
    }
    
    return false;
}

bool RenderController::saveScreenshot(const std::string& filePath, RenderFormat format) {
    if (renderEngine_) {
        return renderEngine_->saveScreenshot(filePath, format);
    }
    
    return false;
}

size_t RenderController::getTriangleCount() const {
    if (renderEngine_) {
        return renderEngine_->getTriangleCount();
    }
    
    return 0;
}

size_t RenderController::getVertexCount() const {
    if (renderEngine_) {
        return renderEngine_->getVertexCount();
    }
    
    return 0;
}

double RenderController::getLastRenderTime() const {
    if (renderEngine_) {
        return renderEngine_->getLastRenderTime();
    }
    
    return 0.0;
}

size_t RenderController::getVideoMemoryUsage() const {
    if (renderEngine_) {
        return renderEngine_->getVideoMemoryUsage();
    }
    
    return 0;
}

RenderController::RenderStatistics RenderController::getRenderStatistics() const {
    RenderStatistics stats;
    
    if (renderEngine_) {
        stats.triangleCount = renderEngine_->getTriangleCount();
        stats.vertexCount = renderEngine_->getVertexCount();
        stats.lastRenderTime = renderEngine_->getLastRenderTime();
        stats.videoMemoryUsage = renderEngine_->getVideoMemoryUsage();
        stats.lightCount = renderEngine_->getLightIds().size();
    }
    
    // Calculate frame rate
    if (stats.lastRenderTime > 0.0) {
        stats.frameRate = static_cast<int>(1000.0 / stats.lastRenderTime);
    }
    
    return stats;
}

void RenderController::setAnimationTime(double time) {
    animationTime_ = time;
}

double RenderController::getAnimationTime() const {
    return animationTime_;
}

void RenderController::setAnimationEnabled(bool enabled) {
    animationEnabled_ = enabled;
}

Camera3D RenderController::calculateOptimalCamera() const {
    BoundingBox bounds = getSceneBounds();
    
    Camera3D camera = currentCamera_;
    calculateCameraParameters(bounds, camera);
    
    return camera;
}

BoundingBox RenderController::getSceneBounds() const {
    if (sceneManager_) {
        return sceneManager_->getSceneBounds();
    }
    
    if (currentProject_) {
        return currentProject_->calculateBoundingBox();
    }
    
    return BoundingBox();
}

bool RenderController::isPointVisible(const Point3D& point) const {
    // This would check if a point is within the camera frustum
    // For now, return a placeholder implementation
    return true;
}

void RenderController::initializePredefinedViews() {
    // Top view (looking down Z-axis)
    Camera3D topView;
    topView.setPosition(Point3D(0.0, 0.0, 10.0));
    topView.setTarget(Point3D(0.0, 0.0, 0.0));
    topView.setUp(Vector3D(0.0, 1.0, 0.0));
    topView.setFOV(45.0);
    predefinedCameras_["Top"] = topView;
    
    // Front view (looking along Y-axis)
    Camera3D frontView;
    frontView.setPosition(Point3D(0.0, -10.0, 0.0));
    frontView.setTarget(Point3D(0.0, 0.0, 0.0));
    frontView.setUp(Vector3D(0.0, 0.0, 1.0));
    frontView.setFOV(45.0);
    predefinedCameras_["Front"] = frontView;
    
    // Right view (looking along X-axis)
    Camera3D rightView;
    rightView.setPosition(Point3D(10.0, 0.0, 0.0));
    rightView.setTarget(Point3D(0.0, 0.0, 0.0));
    rightView.setUp(Vector3D(0.0, 0.0, 1.0));
    rightView.setFOV(45.0);
    predefinedCameras_["Right"] = rightView;
    
    // Isometric view
    Camera3D isometricView;
    isometricView.setPosition(Point3D(7.07, 7.07, 7.07));
    isometricView.setTarget(Point3D(0.0, 0.0, 0.0));
    isometricView.setUp(Vector3D(0.0, 0.0, 1.0));
    isometricView.setFOV(45.0);
    predefinedCameras_["Isometric"] = isometricView;
}

void RenderController::updateRenderEngine() {
    if (renderEngine_) {
        renderEngine_->setCamera(currentCamera_);
        renderEngine_->setRenderSettings(currentSettings_);
    }
}

std::vector<SceneObject*> RenderController::getSceneObjects() const {
    std::vector<SceneObject*> objects;
    
    if (sceneManager_) {
        sceneManager_->forEachObject([&objects](const ObjectId& id, SceneObject* object) {
            if (object) {
                objects.push_back(object);
            }
        });
    }
    
    return objects;
}

void RenderController::notifyRenderCompleted() {
    if (renderCompletedCallback_) {
        renderCompletedCallback_();
    }
}

void RenderController::notifyViewChanged(const std::string& viewName) {
    if (viewChangedCallback_) {
        viewChangedCallback_(viewName);
    }
}

void RenderController::notifyError(const std::string& error) {
    if (errorCallback_) {
        errorCallback_(error);
    }
}

void RenderController::calculateCameraParameters(const BoundingBox& bounds, Camera3D& camera) const {
    if (bounds.isEmpty()) {
        return;
    }
    
    Point3D center = bounds.center();
    Vector3D size = bounds.size();
    double maxDimension = std::max({size.x, size.y, size.z});
    
    // Calculate distance to fit the scene
    double fov = camera.getFOV() * M_PI / 180.0; // Convert to radians
    double distance = maxDimension / (2.0 * std::tan(fov / 2.0)) * 1.5; // Add some margin
    
    // Keep current camera direction but adjust distance
    Vector3D direction = (camera.getPosition() - camera.getTarget()).normalized();
    
    camera.setTarget(center);
    camera.setPosition(center + direction * distance);
}

std::string RenderController::generateLightId() const {
    static int lightCounter = 0;
    return "light_" + std::to_string(++lightCounter);
}

} // namespace Controllers
} // namespace KitchenCAD