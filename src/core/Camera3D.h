#pragma once

#include "../geometry/Point3D.h"
#include "../geometry/Vector3D.h"
#include "../geometry/Matrix4x4.h"
#include <cmath>

namespace KitchenCAD {

/**
 * @brief Camera projection types
 */
enum class ProjectionType {
    Perspective,
    Orthographic
};

/**
 * @brief 3D camera for scene viewing and rendering
 * 
 * This class represents a 3D camera with position, orientation,
 * and projection parameters for viewing the scene.
 */
class Camera3D {
private:
    Geometry::Point3D position_;
    Geometry::Point3D target_;
    Geometry::Vector3D up_;
    
    ProjectionType projectionType_;
    double fieldOfView_;        // In radians for perspective
    double orthographicSize_;   // Height for orthographic
    double nearPlane_;
    double farPlane_;
    double aspectRatio_;
    
public:
    // Constructors
    Camera3D() 
        : position_(0, 0, 5)
        , target_(0, 0, 0)
        , up_(0, 1, 0)
        , projectionType_(ProjectionType::Perspective)
        , fieldOfView_(M_PI / 4.0)  // 45 degrees
        , orthographicSize_(10.0)
        , nearPlane_(0.1)
        , farPlane_(1000.0)
        , aspectRatio_(16.0 / 9.0)
    {}
    
    Camera3D(const Geometry::Point3D& position, const Geometry::Point3D& target)
        : Camera3D()
    {
        position_ = position;
        target_ = target;
    }
    
    // Position and orientation
    void setPosition(const Geometry::Point3D& position) { position_ = position; }
    const Geometry::Point3D& getPosition() const { return position_; }
    
    void setTarget(const Geometry::Point3D& target) { target_ = target; }
    const Geometry::Point3D& getTarget() const { return target_; }
    
    void setUp(const Geometry::Vector3D& up) { up_ = up.normalized(); }
    const Geometry::Vector3D& getUp() const { return up_; }
    
    // Derived vectors
    Geometry::Vector3D getForward() const {
        return Geometry::Vector3D(position_, target_).normalized();
    }
    
    Geometry::Vector3D getRight() const {
        return getForward().cross(up_).normalized();
    }
    
    // Projection settings
    void setProjectionType(ProjectionType type) { projectionType_ = type; }
    ProjectionType getProjectionType() const { return projectionType_; }
    
    void setFieldOfView(double fov) { fieldOfView_ = fov; }
    double getFieldOfView() const { return fieldOfView_; }
    
    void setOrthographicSize(double size) { orthographicSize_ = size; }
    double getOrthographicSize() const { return orthographicSize_; }
    
    void setNearPlane(double near) { nearPlane_ = near; }
    double getNearPlane() const { return nearPlane_; }
    
    void setFarPlane(double far) { farPlane_ = far; }
    double getFarPlane() const { return farPlane_; }
    
    void setAspectRatio(double ratio) { aspectRatio_ = ratio; }
    double getAspectRatio() const { return aspectRatio_; }
    
    // Camera movement
    void moveForward(double distance) {
        Geometry::Vector3D forward = getForward();
        position_ = position_ + forward * distance;
        target_ = target_ + forward * distance;
    }
    
    void moveRight(double distance) {
        Geometry::Vector3D right = getRight();
        position_ = position_ + right * distance;
        target_ = target_ + right * distance;
    }
    
    void moveUp(double distance) {
        position_ = position_ + up_ * distance;
        target_ = target_ + up_ * distance;
    }
    
    void orbit(double horizontalAngle, double verticalAngle) {
        Geometry::Vector3D toCamera = Geometry::Vector3D(target_, position_);
        double distance = toCamera.length();
        
        // Convert to spherical coordinates
        double theta = std::atan2(toCamera.x, toCamera.z) + horizontalAngle;
        double phi = std::acos(toCamera.y / distance) + verticalAngle;
        
        // Clamp phi to avoid gimbal lock
        phi = std::max(0.01, std::min(M_PI - 0.01, phi));
        
        // Convert back to Cartesian
        double x = distance * std::sin(phi) * std::sin(theta);
        double y = distance * std::cos(phi);
        double z = distance * std::sin(phi) * std::cos(theta);
        
        position_ = target_ + Geometry::Vector3D(x, y, z);
    }
    
    void zoom(double factor) {
        if (projectionType_ == ProjectionType::Perspective) {
            Geometry::Vector3D toTarget = Geometry::Vector3D(position_, target_);
            double distance = toTarget.length();
            distance *= factor;
            position_ = target_ + toTarget.normalized() * distance;
        } else {
            orthographicSize_ *= factor;
        }
    }
    
    // Matrix generation
    Geometry::Matrix4x4 getViewMatrix() const {
        return Geometry::Matrix4x4::lookAt(position_, target_, up_);
    }
    
    Geometry::Matrix4x4 getProjectionMatrix() const {
        if (projectionType_ == ProjectionType::Perspective) {
            return Geometry::Matrix4x4::perspective(fieldOfView_, aspectRatio_, nearPlane_, farPlane_);
        } else {
            double width = orthographicSize_ * aspectRatio_;
            double height = orthographicSize_;
            return Geometry::Matrix4x4::orthographic(-width/2, width/2, -height/2, height/2, nearPlane_, farPlane_);
        }
    }
    
    Geometry::Matrix4x4 getViewProjectionMatrix() const {
        return getProjectionMatrix() * getViewMatrix();
    }
    
    // Ray casting
    struct Ray {
        Geometry::Point3D origin;
        Geometry::Vector3D direction;
    };
    
    Ray screenToRay(double screenX, double screenY, int screenWidth, int screenHeight) const {
        // Convert screen coordinates to normalized device coordinates [-1, 1]
        double x = (2.0 * screenX) / screenWidth - 1.0;
        double y = 1.0 - (2.0 * screenY) / screenHeight;
        
        Ray ray;
        ray.origin = position_;
        
        if (projectionType_ == ProjectionType::Perspective) {
            // Calculate ray direction for perspective projection
            Geometry::Vector3D forward = getForward();
            Geometry::Vector3D right = getRight();
            Geometry::Vector3D up = up_;
            
            double tanHalfFov = std::tan(fieldOfView_ * 0.5);
            Geometry::Vector3D direction = forward + 
                                         right * (x * tanHalfFov * aspectRatio_) + 
                                         up * (y * tanHalfFov);
            ray.direction = direction.normalized();
        } else {
            // For orthographic projection, all rays are parallel
            ray.direction = getForward();
            
            // Adjust ray origin for orthographic projection
            Geometry::Vector3D right = getRight();
            Geometry::Vector3D up = up_;
            double width = orthographicSize_ * aspectRatio_;
            double height = orthographicSize_;
            
            ray.origin = position_ + right * (x * width * 0.5) + up * (y * height * 0.5);
        }
        
        return ray;
    }
    
    // Predefined camera positions
    static Camera3D createIsometric(const Geometry::Point3D& target, double distance) {
        Camera3D camera;
        camera.setTarget(target);
        camera.setPosition(target + Geometry::Vector3D(distance, distance, distance));
        camera.setUp(Geometry::Vector3D(0, 1, 0));
        return camera;
    }
    
    static Camera3D createFront(const Geometry::Point3D& target, double distance) {
        Camera3D camera;
        camera.setTarget(target);
        camera.setPosition(target + Geometry::Vector3D(0, 0, distance));
        camera.setUp(Geometry::Vector3D(0, 1, 0));
        return camera;
    }
    
    static Camera3D createTop(const Geometry::Point3D& target, double distance) {
        Camera3D camera;
        camera.setTarget(target);
        camera.setPosition(target + Geometry::Vector3D(0, distance, 0));
        camera.setUp(Geometry::Vector3D(0, 0, -1));
        return camera;
    }
    
    static Camera3D createRight(const Geometry::Point3D& target, double distance) {
        Camera3D camera;
        camera.setTarget(target);
        camera.setPosition(target + Geometry::Vector3D(distance, 0, 0));
        camera.setUp(Geometry::Vector3D(0, 1, 0));
        return camera;
    }
};

} // namespace KitchenCAD