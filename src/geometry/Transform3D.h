#pragma once

#include "Point3D.h"
#include "Vector3D.h"
#include "Matrix4x4.h"
#include <cmath>

namespace KitchenCAD {
namespace Geometry {

/**
 * @brief Represents a 3D transformation (translation, rotation, scale)
 */
struct Transform3D {
    Point3D translation;
    Vector3D rotation;  // Euler angles in radians (X, Y, Z order)
    Vector3D scale;
    
    // Constructors
    Transform3D() : scale(1.0, 1.0, 1.0) {}
    
    Transform3D(const Point3D& translation, const Vector3D& rotation = Vector3D(), const Vector3D& scale = Vector3D(1.0, 1.0, 1.0))
        : translation(translation), rotation(rotation), scale(scale) {}
    
    // Comparison operators
    bool operator==(const Transform3D& other) const {
        return translation == other.translation &&
               rotation == other.rotation &&
               scale == other.scale;
    }
    
    bool operator!=(const Transform3D& other) const {
        return !(*this == other);
    }
    
    // Transform operations
    Matrix4x4 toMatrix() const {
        // Create transformation matrix: T * R * S
        Matrix4x4 scaleMatrix = Matrix4x4::scale(scale);
        Matrix4x4 rotationMatrix = Matrix4x4::rotationX(rotation.x) *
                                   Matrix4x4::rotationY(rotation.y) *
                                   Matrix4x4::rotationZ(rotation.z);
        Matrix4x4 translationMatrix = Matrix4x4::translation(Vector3D(translation.x, translation.y, translation.z));
        
        return translationMatrix * rotationMatrix * scaleMatrix;
    }
    
    Transform3D inverse() const {
        // Inverse transformation: S^-1 * R^-1 * T^-1
        Vector3D invScale(
            std::abs(scale.x) > 1e-9 ? 1.0 / scale.x : 0.0,
            std::abs(scale.y) > 1e-9 ? 1.0 / scale.y : 0.0,
            std::abs(scale.z) > 1e-9 ? 1.0 / scale.z : 0.0
        );
        
        Vector3D invRotation(-rotation.x, -rotation.y, -rotation.z);
        
        // Apply inverse rotation to translation
        Matrix4x4 invRotMatrix = Matrix4x4::rotationZ(-rotation.z) *
                                Matrix4x4::rotationY(-rotation.y) *
                                Matrix4x4::rotationX(-rotation.x);
        Vector3D invTranslation = invRotMatrix.transformVector(Vector3D(-translation.x, -translation.y, -translation.z));
        
        // Scale the translation
        invTranslation.x *= invScale.x;
        invTranslation.y *= invScale.y;
        invTranslation.z *= invScale.z;
        
        return Transform3D(invTranslation.toPoint(), invRotation, invScale);
    }
    
    Transform3D operator*(const Transform3D& other) const {
        Matrix4x4 thisMatrix = toMatrix();
        Matrix4x4 otherMatrix = other.toMatrix();
        Matrix4x4 resultMatrix = thisMatrix * otherMatrix;
        
        // Extract transformation components from result matrix
        // This is a simplified extraction - for production code, you might want
        // to use more robust matrix decomposition
        Point3D resultTranslation(resultMatrix(0, 3), resultMatrix(1, 3), resultMatrix(2, 3));
        
        // For simplicity, combine rotations and scales
        Vector3D resultRotation = rotation + other.rotation;
        Vector3D resultScale(scale.x * other.scale.x, scale.y * other.scale.y, scale.z * other.scale.z);
        
        return Transform3D(resultTranslation, resultRotation, resultScale);
    }
    
    // Transform points and vectors
    Point3D transformPoint(const Point3D& point) const {
        return toMatrix().transformPoint(point);
    }
    
    Vector3D transformVector(const Vector3D& vector) const {
        return toMatrix().transformVector(vector);
    }
    
    // Utility methods
    void setTranslation(const Point3D& newTranslation) {
        translation = newTranslation;
    }
    
    void setRotation(const Vector3D& newRotation) {
        rotation = newRotation;
    }
    
    void setScale(const Vector3D& newScale) {
        scale = newScale;
    }
    
    void translate(const Vector3D& delta) {
        translation = translation + delta;
    }
    
    void rotate(const Vector3D& deltaRotation) {
        rotation += deltaRotation;
    }
    
    void scaleBy(const Vector3D& scaleFactors) {
        scale.x *= scaleFactors.x;
        scale.y *= scaleFactors.y;
        scale.z *= scaleFactors.z;
    }
    
    // Reset to identity
    void setIdentity() {
        translation = Point3D();
        rotation = Vector3D();
        scale = Vector3D(1.0, 1.0, 1.0);
    }
    
    bool isIdentity(double epsilon = 1e-9) const {
        return translation.distanceTo(Point3D()) < epsilon &&
               rotation.length() < epsilon &&
               std::abs(scale.x - 1.0) < epsilon &&
               std::abs(scale.y - 1.0) < epsilon &&
               std::abs(scale.z - 1.0) < epsilon;
    }
    
    // Static factory methods
    static Transform3D identity() {
        return Transform3D();
    }
    
    static Transform3D fromTranslation(const Vector3D& translation) {
        return Transform3D(translation.toPoint());
    }
    
    static Transform3D fromRotation(const Vector3D& rotation) {
        return Transform3D(Point3D(), rotation);
    }
    
    static Transform3D fromScale(const Vector3D& scale) {
        return Transform3D(Point3D(), Vector3D(), scale);
    }
    
    // Interpolation
    Transform3D lerp(const Transform3D& other, double t) const {
        t = std::max(0.0, std::min(1.0, t));  // Clamp t to [0, 1]
        
        Point3D lerpTranslation(
            translation.x + t * (other.translation.x - translation.x),
            translation.y + t * (other.translation.y - translation.y),
            translation.z + t * (other.translation.z - translation.z)
        );
        
        Vector3D lerpRotation(
            rotation.x + t * (other.rotation.x - rotation.x),
            rotation.y + t * (other.rotation.y - rotation.y),
            rotation.z + t * (other.rotation.z - rotation.z)
        );
        
        Vector3D lerpScale(
            scale.x + t * (other.scale.x - scale.x),
            scale.y + t * (other.scale.y - scale.y),
            scale.z + t * (other.scale.z - scale.z)
        );
        
        return Transform3D(lerpTranslation, lerpRotation, lerpScale);
    }
};

} // namespace Geometry
} // namespace KitchenCAD