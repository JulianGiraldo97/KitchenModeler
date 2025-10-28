#pragma once

#include <cmath>
#include <array>

namespace KitchenCAD {
namespace Geometry {



/**
 * @brief Represents a point in 3D space
 */
struct Point3D {
    double x, y, z;
    
    // Constructors
    Point3D() : x(0.0), y(0.0), z(0.0) {}
    Point3D(double x, double y, double z) : x(x), y(y), z(z) {}
    
    // Arithmetic operations
    Point3D operator+(const Point3D& other) const {
        return Point3D(x + other.x, y + other.y, z + other.z);
    }
    
    Point3D operator-(const Point3D& other) const {
        return Point3D(x - other.x, y - other.y, z - other.z);
    }
    
    Point3D operator*(double scalar) const {
        return Point3D(x * scalar, y * scalar, z * scalar);
    }
    
    Point3D operator/(double scalar) const {
        return Point3D(x / scalar, y / scalar, z / scalar);
    }
    
    // Compound assignment operators
    Point3D& operator+=(const Point3D& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }
    
    Point3D& operator-=(const Point3D& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }
    

    
    Point3D& operator*=(double scalar) {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }
    
    // Comparison operators
    bool operator==(const Point3D& other) const {
        const double epsilon = 1e-9;
        return std::abs(x - other.x) < epsilon &&
               std::abs(y - other.y) < epsilon &&
               std::abs(z - other.z) < epsilon;
    }
    
    bool operator!=(const Point3D& other) const {
        return !(*this == other);
    }
    
    // Utility methods
    double distanceTo(const Point3D& other) const {
        double dx = x - other.x;
        double dy = y - other.y;
        double dz = z - other.z;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }
    
    double distanceSquaredTo(const Point3D& other) const {
        double dx = x - other.x;
        double dy = y - other.y;
        double dz = z - other.z;
        return dx * dx + dy * dy + dz * dz;
    }
    
    // Array access
    double& operator[](int index) {
        return (&x)[index];
    }
    
    const double& operator[](int index) const {
        return (&x)[index];
    }
    
    // Convert to array
    std::array<double, 3> toArray() const {
        return {x, y, z};
    }
};

// Global operators
inline Point3D operator*(double scalar, const Point3D& point) {
    return point * scalar;
}

} // namespace Geometry
} // namespace KitchenCAD