#pragma once

#include "Point3D.h"
#include "Vector3D.h"
#include "Transform3D.h"
#include <algorithm>
#include <vector>
#include <limits>

namespace KitchenCAD {
namespace Geometry {

/**
 * @brief Axis-aligned bounding box in 3D space
 */
struct BoundingBox {
    Point3D min, max;
    
    // Constructors
    BoundingBox() {
        reset();
    }
    
    BoundingBox(const Point3D& min, const Point3D& max) : min(min), max(max) {
        normalize();
    }
    
    BoundingBox(const Point3D& center, double width, double height, double depth) {
        double halfWidth = width * 0.5;
        double halfHeight = height * 0.5;
        double halfDepth = depth * 0.5;
        
        min = Point3D(center.x - halfWidth, center.y - halfHeight, center.z - halfDepth);
        max = Point3D(center.x + halfWidth, center.y + halfHeight, center.z + halfDepth);
    }
    
    // Comparison operators
    bool operator==(const BoundingBox& other) const {
        return min == other.min && max == other.max;
    }
    
    bool operator!=(const BoundingBox& other) const {
        return !(*this == other);
    }
    
    // Utility methods
    void reset() {
        const double inf = std::numeric_limits<double>::infinity();
        min = Point3D(inf, inf, inf);
        max = Point3D(-inf, -inf, -inf);
    }
    
    void normalize() {
        if (min.x > max.x) std::swap(min.x, max.x);
        if (min.y > max.y) std::swap(min.y, max.y);
        if (min.z > max.z) std::swap(min.z, max.z);
    }
    
    bool isValid() const {
        return min.x <= max.x && min.y <= max.y && min.z <= max.z &&
               std::isfinite(min.x) && std::isfinite(min.y) && std::isfinite(min.z) &&
               std::isfinite(max.x) && std::isfinite(max.y) && std::isfinite(max.z);
    }
    
    bool isEmpty() const {
        const double inf = std::numeric_limits<double>::infinity();
        return min.x == inf || min.y == inf || min.z == inf ||
               max.x == -inf || max.y == -inf || max.z == -inf;
    }
    
    Point3D center() const {
        return Point3D(
            (min.x + max.x) * 0.5,
            (min.y + max.y) * 0.5,
            (min.z + max.z) * 0.5
        );
    }
    
    Vector3D size() const {
        return Vector3D(max.x - min.x, max.y - min.y, max.z - min.z);
    }
    
    double width() const { return max.x - min.x; }
    double height() const { return max.y - min.y; }
    double depth() const { return max.z - min.z; }
    
    double volume() const {
        if (!isValid() || isEmpty()) return 0.0;
        Vector3D s = size();
        return s.x * s.y * s.z;
    }
    
    double surfaceArea() const {
        if (!isValid() || isEmpty()) return 0.0;
        Vector3D s = size();
        return 2.0 * (s.x * s.y + s.y * s.z + s.z * s.x);
    }
    
    // Containment tests
    bool contains(const Point3D& point) const {
        return point.x >= min.x && point.x <= max.x &&
               point.y >= min.y && point.y <= max.y &&
               point.z >= min.z && point.z <= max.z;
    }
    
    bool contains(const BoundingBox& other) const {
        return contains(other.min) && contains(other.max);
    }
    
    bool intersects(const BoundingBox& other) const {
        return !(max.x < other.min.x || min.x > other.max.x ||
                 max.y < other.min.y || min.y > other.max.y ||
                 max.z < other.min.z || min.z > other.max.z);
    }
    
    // Expansion and modification
    void expand(const Point3D& point) {
        if (isEmpty()) {
            min = max = point;
        } else {
            min.x = std::min(min.x, point.x);
            min.y = std::min(min.y, point.y);
            min.z = std::min(min.z, point.z);
            max.x = std::max(max.x, point.x);
            max.y = std::max(max.y, point.y);
            max.z = std::max(max.z, point.z);
        }
    }
    
    void expand(const BoundingBox& other) {
        if (other.isEmpty()) return;
        if (isEmpty()) {
            *this = other;
        } else {
            expand(other.min);
            expand(other.max);
        }
    }
    
    void expand(double amount) {
        if (isEmpty()) return;
        Vector3D expansion(amount, amount, amount);
        min = min - expansion;
        max = max + expansion;
    }
    
    BoundingBox expanded(double amount) const {
        BoundingBox result = *this;
        result.expand(amount);
        return result;
    }
    
    // Intersection and union
    BoundingBox intersection(const BoundingBox& other) const {
        if (!intersects(other)) {
            return BoundingBox();  // Empty box
        }
        
        return BoundingBox(
            Point3D(
                std::max(min.x, other.min.x),
                std::max(min.y, other.min.y),
                std::max(min.z, other.min.z)
            ),
            Point3D(
                std::min(max.x, other.max.x),
                std::min(max.y, other.max.y),
                std::min(max.z, other.max.z)
            )
        );
    }
    
    BoundingBox unionWith(const BoundingBox& other) const {
        if (isEmpty()) return other;
        if (other.isEmpty()) return *this;
        
        BoundingBox result = *this;
        result.expand(other);
        return result;
    }
    
    // Distance calculations
    double distanceTo(const Point3D& point) const {
        if (contains(point)) return 0.0;
        
        double dx = std::max(0.0, std::max(min.x - point.x, point.x - max.x));
        double dy = std::max(0.0, std::max(min.y - point.y, point.y - max.y));
        double dz = std::max(0.0, std::max(min.z - point.z, point.z - max.z));
        
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }
    
    double distanceSquaredTo(const Point3D& point) const {
        if (contains(point)) return 0.0;
        
        double dx = std::max(0.0, std::max(min.x - point.x, point.x - max.x));
        double dy = std::max(0.0, std::max(min.y - point.y, point.y - max.y));
        double dz = std::max(0.0, std::max(min.z - point.z, point.z - max.z));
        
        return dx * dx + dy * dy + dz * dz;
    }
    
    // Transformation
    BoundingBox transformed(const Transform3D& transform) const {
        if (isEmpty()) return *this;
        
        // Transform all 8 corners of the bounding box
        std::vector<Point3D> corners = {
            Point3D(min.x, min.y, min.z),
            Point3D(max.x, min.y, min.z),
            Point3D(min.x, max.y, min.z),
            Point3D(max.x, max.y, min.z),
            Point3D(min.x, min.y, max.z),
            Point3D(max.x, min.y, max.z),
            Point3D(min.x, max.y, max.z),
            Point3D(max.x, max.y, max.z)
        };
        
        BoundingBox result;
        for (const auto& corner : corners) {
            Point3D transformedCorner = transform.transformPoint(corner);
            result.expand(transformedCorner);
        }
        
        return result;
    }
    
    // Get corner points
    std::vector<Point3D> getCorners() const {
        return {
            Point3D(min.x, min.y, min.z),  // 0: min corner
            Point3D(max.x, min.y, min.z),  // 1: +X
            Point3D(min.x, max.y, min.z),  // 2: +Y
            Point3D(max.x, max.y, min.z),  // 3: +X+Y
            Point3D(min.x, min.y, max.z),  // 4: +Z
            Point3D(max.x, min.y, max.z),  // 5: +X+Z
            Point3D(min.x, max.y, max.z),  // 6: +Y+Z
            Point3D(max.x, max.y, max.z)   // 7: max corner
        };
    }
    
    // Closest point on or in the box
    Point3D closestPoint(const Point3D& point) const {
        return Point3D(
            std::max(min.x, std::min(max.x, point.x)),
            std::max(min.y, std::min(max.y, point.y)),
            std::max(min.z, std::min(max.z, point.z))
        );
    }
    
    // Static factory methods
    static BoundingBox fromPoints(const std::vector<Point3D>& points) {
        BoundingBox result;
        for (const auto& point : points) {
            result.expand(point);
        }
        return result;
    }
    
    static BoundingBox fromCenterAndSize(const Point3D& center, const Vector3D& size) {
        Vector3D halfSize = size * 0.5;
        return BoundingBox(center - halfSize, center + halfSize);
    }
};

} // namespace Geometry
} // namespace KitchenCAD