#pragma once

/**
 * @file Geometry.h
 * @brief Main header for all geometric types and utilities
 * 
 * This header provides a convenient way to include all geometric types
 * and utilities used throughout the Kitchen CAD Designer application.
 */

// Core geometric types
#include "Point3D.h"
#include "Vector3D.h"
#include "Matrix4x4.h"
#include "Transform3D.h"
#include "BoundingBox.h"

// Utility classes
#include "GeometryUtils.h"

namespace KitchenCAD {
namespace Geometry {

/**
 * @brief Common type aliases for convenience
 */
using Point = Point3D;
using Vector = Vector3D;
using Matrix = Matrix4x4;
using Transform = Transform3D;
using BBox = BoundingBox;

/**
 * @brief Dimensions structure for 3D objects
 */
struct Dimensions3D {
    double width, height, depth;
    
    Dimensions3D() : width(0.0), height(0.0), depth(0.0) {}
    Dimensions3D(double w, double h, double d) : width(w), height(h), depth(d) {}
    
    Vector3D toVector() const {
        return Vector3D(width, height, depth);
    }
    
    BoundingBox toBoundingBox(const Point3D& center = Point3D()) const {
        return BoundingBox::fromCenterAndSize(center, toVector());
    }
    
    double volume() const {
        return width * height * depth;
    }
    
    bool isValid() const {
        return width > 0.0 && height > 0.0 && depth > 0.0;
    }
    
    bool operator==(const Dimensions3D& other) const {
        return GeometryUtils::isEqual(width, other.width) &&
               GeometryUtils::isEqual(height, other.height) &&
               GeometryUtils::isEqual(depth, other.depth);
    }
    
    bool operator!=(const Dimensions3D& other) const {
        return !(*this == other);
    }
};

/**
 * @brief Color structure for materials and rendering
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
    static Color yellow() { return Color(1.0f, 1.0f, 0.0f); }
    static Color cyan() { return Color(0.0f, 1.0f, 1.0f); }
    static Color magenta() { return Color(1.0f, 0.0f, 1.0f); }
    static Color gray(float intensity = 0.5f) { return Color(intensity, intensity, intensity); }
    
    // Color operations
    Color operator+(const Color& other) const {
        return Color(r + other.r, g + other.g, b + other.b, a + other.a);
    }
    
    Color operator*(float scalar) const {
        return Color(r * scalar, g * scalar, b * scalar, a * scalar);
    }
    
    Color lerp(const Color& other, float t) const {
        t = std::max(0.0f, std::min(1.0f, t));
        return Color(
            r + t * (other.r - r),
            g + t * (other.g - g),
            b + t * (other.b - b),
            a + t * (other.a - a)
        );
    }
};

/**
 * @brief Ray structure for ray casting and intersection tests
 */
struct Ray {
    Point3D origin;
    Vector3D direction;
    
    Ray() = default;
    Ray(const Point3D& origin, const Vector3D& direction) 
        : origin(origin), direction(direction.normalized()) {}
    
    Point3D pointAt(double t) const {
        return origin + direction * t;
    }
    
    // Ray-bounding box intersection
    bool intersects(const BoundingBox& bbox, double& tMin, double& tMax) const {
        tMin = 0.0;
        tMax = std::numeric_limits<double>::infinity();
        
        for (int i = 0; i < 3; ++i) {
            double invDir = 1.0 / direction[i];
            double t1 = (bbox.min[i] - origin[i]) * invDir;
            double t2 = (bbox.max[i] - origin[i]) * invDir;
            
            if (invDir < 0.0) {
                std::swap(t1, t2);
            }
            
            tMin = std::max(tMin, t1);
            tMax = std::min(tMax, t2);
            
            if (tMin > tMax) {
                return false;
            }
        }
        
        return true;
    }
    
    bool intersects(const BoundingBox& bbox) const {
        double tMin, tMax;
        return intersects(bbox, tMin, tMax);
    }
};

/**
 * @brief Utility functions for common geometric operations
 */
namespace GeometryOps {
    
    // Create a bounding box from a set of points
    inline BoundingBox createBoundingBox(const std::vector<Point3D>& points) {
        return BoundingBox::fromPoints(points);
    }
    
    // Create a transform from position, rotation (in degrees), and scale
    inline Transform3D createTransform(const Point3D& position, 
                                      const Vector3D& rotationDegrees = Vector3D(),
                                      const Vector3D& scale = Vector3D(1, 1, 1)) {
        Vector3D rotationRadians = GeometryUtils::degreesToRadians(rotationDegrees);
        return Transform3D(position, rotationRadians, scale);
    }
    
    // Calculate the center point of multiple points
    inline Point3D calculateCenter(const std::vector<Point3D>& points) {
        if (points.empty()) return Point3D();
        
        Point3D sum;
        for (const auto& point : points) {
            sum += point;
        }
        return sum / static_cast<double>(points.size());
    }
    
    // Check if two bounding boxes overlap
    inline bool boundingBoxesOverlap(const BoundingBox& a, const BoundingBox& b) {
        return a.intersects(b);
    }
    
    // Calculate the distance between two bounding boxes
    inline double boundingBoxDistance(const BoundingBox& a, const BoundingBox& b) {
        if (a.intersects(b)) return 0.0;
        
        Point3D centerA = a.center();
        Point3D centerB = b.center();
        
        // Find closest points on each bounding box
        Point3D closestA = a.closestPoint(centerB);
        Point3D closestB = b.closestPoint(centerA);
        
        return closestA.distanceTo(closestB);
    }
}

} // namespace Geometry
} // namespace KitchenCAD