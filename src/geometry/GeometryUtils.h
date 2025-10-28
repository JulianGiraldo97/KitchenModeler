#pragma once

#include "Point3D.h"
#include "Vector3D.h"
#include "Transform3D.h"
#include "BoundingBox.h"
#include <vector>
#include <cmath>

namespace KitchenCAD {
namespace Geometry {

/**
 * @brief Utility class for geometric calculations and conversions
 */
class GeometryUtils {
public:
    // Constants
    static constexpr double PI = 3.14159265358979323846;
    static constexpr double TWO_PI = 2.0 * PI;
    static constexpr double HALF_PI = PI * 0.5;
    static constexpr double DEG_TO_RAD = PI / 180.0;
    static constexpr double RAD_TO_DEG = 180.0 / PI;
    static constexpr double EPSILON = 1e-9;
    
    // Angle conversions
    static double degreesToRadians(double degrees) {
        return degrees * DEG_TO_RAD;
    }
    
    static double radiansToDegrees(double radians) {
        return radians * RAD_TO_DEG;
    }
    
    static Vector3D degreesToRadians(const Vector3D& degrees) {
        return Vector3D(
            degrees.x * DEG_TO_RAD,
            degrees.y * DEG_TO_RAD,
            degrees.z * DEG_TO_RAD
        );
    }
    
    static Vector3D radiansToDegrees(const Vector3D& radians) {
        return Vector3D(
            radians.x * RAD_TO_DEG,
            radians.y * RAD_TO_DEG,
            radians.z * RAD_TO_DEG
        );
    }
    
    // Normalize angle to [0, 2π) range
    static double normalizeAngle(double angle) {
        while (angle < 0.0) angle += TWO_PI;
        while (angle >= TWO_PI) angle -= TWO_PI;
        return angle;
    }
    
    // Normalize angle to [-π, π) range
    static double normalizeAngleSigned(double angle) {
        while (angle < -PI) angle += TWO_PI;
        while (angle >= PI) angle -= TWO_PI;
        return angle;
    }
    
    // Floating point comparisons
    static bool isEqual(double a, double b, double epsilon = EPSILON) {
        return std::abs(a - b) < epsilon;
    }
    
    static bool isZero(double value, double epsilon = EPSILON) {
        return std::abs(value) < epsilon;
    }
    
    // Clamping
    static double clamp(double value, double min, double max) {
        return std::max(min, std::min(max, value));
    }
    
    static Point3D clamp(const Point3D& point, const BoundingBox& bounds) {
        return Point3D(
            clamp(point.x, bounds.min.x, bounds.max.x),
            clamp(point.y, bounds.min.y, bounds.max.y),
            clamp(point.z, bounds.min.z, bounds.max.z)
        );
    }
    
    // Interpolation
    static double lerp(double a, double b, double t) {
        return a + t * (b - a);
    }
    
    static Point3D lerp(const Point3D& a, const Point3D& b, double t) {
        return Point3D(
            lerp(a.x, b.x, t),
            lerp(a.y, b.y, t),
            lerp(a.z, b.z, t)
        );
    }
    
    static Vector3D lerp(const Vector3D& a, const Vector3D& b, double t) {
        return Vector3D(
            lerp(a.x, b.x, t),
            lerp(a.y, b.y, t),
            lerp(a.z, b.z, t)
        );
    }
    
    // Smoothstep interpolation (smooth cubic curve)
    static double smoothstep(double edge0, double edge1, double x) {
        double t = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
        return t * t * (3.0 - 2.0 * t);
    }
    
    // Distance calculations
    static double pointToLineDistance(const Point3D& point, const Point3D& lineStart, const Point3D& lineEnd) {
        Vector3D lineDir = Vector3D(lineStart, lineEnd);
        Vector3D pointDir = Vector3D(lineStart, point);
        
        if (lineDir.isZero()) {
            return point.distanceTo(lineStart);
        }
        
        double t = pointDir.dot(lineDir) / lineDir.lengthSquared();
        Point3D closestPoint = lineStart + lineDir * t;
        return point.distanceTo(closestPoint);
    }
    
    static Point3D closestPointOnLine(const Point3D& point, const Point3D& lineStart, const Point3D& lineEnd) {
        Vector3D lineDir = Vector3D(lineStart, lineEnd);
        Vector3D pointDir = Vector3D(lineStart, point);
        
        if (lineDir.isZero()) {
            return lineStart;
        }
        
        double t = pointDir.dot(lineDir) / lineDir.lengthSquared();
        return lineStart + lineDir * t;
    }
    
    static Point3D closestPointOnSegment(const Point3D& point, const Point3D& segmentStart, const Point3D& segmentEnd) {
        Vector3D lineDir = Vector3D(segmentStart, segmentEnd);
        Vector3D pointDir = Vector3D(segmentStart, point);
        
        if (lineDir.isZero()) {
            return segmentStart;
        }
        
        double t = clamp(pointDir.dot(lineDir) / lineDir.lengthSquared(), 0.0, 1.0);
        return segmentStart + lineDir * t;
    }
    
    // Plane calculations
    struct Plane {
        Point3D point;
        Vector3D normal;
        
        Plane(const Point3D& point, const Vector3D& normal) 
            : point(point), normal(normal.normalized()) {}
    };
    
    static Plane planeFromThreePoints(const Point3D& p1, const Point3D& p2, const Point3D& p3) {
        Vector3D v1 = Vector3D(p1, p2);
        Vector3D v2 = Vector3D(p1, p3);
        Vector3D normal = v1.cross(v2).normalized();
        return Plane(p1, normal);
    }
    
    static double distanceToPlane(const Point3D& point, const Plane& plane) {
        Vector3D pointToPlane = Vector3D(plane.point, point);
        return pointToPlane.dot(plane.normal);
    }
    
    static Point3D projectPointOnPlane(const Point3D& point, const Plane& plane) {
        double distance = distanceToPlane(point, plane);
        return point - plane.normal * distance;
    }
    
    // Grid and snapping utilities
    static Point3D snapToGrid(const Point3D& point, double gridSize) {
        if (gridSize <= 0.0) return point;
        
        return Point3D(
            std::round(point.x / gridSize) * gridSize,
            std::round(point.y / gridSize) * gridSize,
            std::round(point.z / gridSize) * gridSize
        );
    }
    
    static double snapToGrid(double value, double gridSize) {
        if (gridSize <= 0.0) return value;
        return std::round(value / gridSize) * gridSize;
    }
    
    // Coordinate system conversions
    struct SphericalCoordinates {
        double radius;
        double theta;    // Azimuthal angle (around Y axis)
        double phi;      // Polar angle (from Y axis)
        
        SphericalCoordinates(double r = 0.0, double t = 0.0, double p = 0.0)
            : radius(r), theta(t), phi(p) {}
    };
    
    static SphericalCoordinates cartesianToSpherical(const Point3D& point) {
        double radius = std::sqrt(point.x * point.x + point.y * point.y + point.z * point.z);
        
        if (radius < EPSILON) {
            return SphericalCoordinates(0.0, 0.0, 0.0);
        }
        
        double theta = std::atan2(point.x, point.z);
        double phi = std::acos(point.y / radius);
        
        return SphericalCoordinates(radius, theta, phi);
    }
    
    static Point3D sphericalToCartesian(const SphericalCoordinates& spherical) {
        double sinPhi = std::sin(spherical.phi);
        return Point3D(
            spherical.radius * sinPhi * std::sin(spherical.theta),
            spherical.radius * std::cos(spherical.phi),
            spherical.radius * sinPhi * std::cos(spherical.theta)
        );
    }
    
    // Bounding box utilities
    static BoundingBox transformBoundingBox(const BoundingBox& bbox, const Transform3D& transform) {
        return bbox.transformed(transform);
    }
    
    static std::vector<BoundingBox> subdivideBoundingBox(const BoundingBox& bbox, int subdivisions) {
        std::vector<BoundingBox> result;
        
        if (subdivisions <= 0 || bbox.isEmpty()) {
            result.push_back(bbox);
            return result;
        }
        
        Vector3D size = bbox.size();
        Vector3D subSize = size / static_cast<double>(subdivisions);
        
        for (int x = 0; x < subdivisions; ++x) {
            for (int y = 0; y < subdivisions; ++y) {
                for (int z = 0; z < subdivisions; ++z) {
                    Point3D subMin = bbox.min + Vector3D(
                        x * subSize.x,
                        y * subSize.y,
                        z * subSize.z
                    );
                    Point3D subMax = subMin + subSize;
                    result.emplace_back(subMin, subMax);
                }
            }
        }
        
        return result;
    }
    
    // Volume and area calculations
    static double triangleArea(const Point3D& p1, const Point3D& p2, const Point3D& p3) {
        Vector3D v1 = Vector3D(p1, p2);
        Vector3D v2 = Vector3D(p1, p3);
        return v1.cross(v2).length() * 0.5;
    }
    
    static double tetrahedronVolume(const Point3D& p1, const Point3D& p2, const Point3D& p3, const Point3D& p4) {
        Vector3D v1 = Vector3D(p1, p2);
        Vector3D v2 = Vector3D(p1, p3);
        Vector3D v3 = Vector3D(p1, p4);
        return std::abs(v1.dot(v2.cross(v3))) / 6.0;
    }
    
    // Collision detection helpers
    static bool sphereIntersectsSphere(const Point3D& center1, double radius1, 
                                      const Point3D& center2, double radius2) {
        double distance = center1.distanceTo(center2);
        return distance <= (radius1 + radius2);
    }
    
    static bool pointInSphere(const Point3D& point, const Point3D& center, double radius) {
        return point.distanceSquaredTo(center) <= (radius * radius);
    }
    
    // Random point generation
    static Point3D randomPointInBoundingBox(const BoundingBox& bbox) {
        if (bbox.isEmpty()) return Point3D();
        
        // Note: This uses a simple linear congruential generator for deterministic results
        // In production code, you might want to use a proper random number generator
        static unsigned int seed = 12345;
        seed = (seed * 1103515245 + 12345) & 0x7fffffff;
        double rx = static_cast<double>(seed) / 0x7fffffff;
        
        seed = (seed * 1103515245 + 12345) & 0x7fffffff;
        double ry = static_cast<double>(seed) / 0x7fffffff;
        
        seed = (seed * 1103515245 + 12345) & 0x7fffffff;
        double rz = static_cast<double>(seed) / 0x7fffffff;
        
        return Point3D(
            bbox.min.x + rx * (bbox.max.x - bbox.min.x),
            bbox.min.y + ry * (bbox.max.y - bbox.min.y),
            bbox.min.z + rz * (bbox.max.z - bbox.min.z)
        );
    }
};

} // namespace Geometry
} // namespace KitchenCAD