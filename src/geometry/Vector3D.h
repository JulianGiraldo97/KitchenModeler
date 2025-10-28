#pragma once

#include "Point3D.h"
#include <cmath>
#include <array>

namespace KitchenCAD {
namespace Geometry {

/**
 * @brief Represents a vector in 3D space
 */
struct Vector3D {
    double x, y, z;
    
    // Constructors
    Vector3D() : x(0.0), y(0.0), z(0.0) {}
    Vector3D(double x, double y, double z) : x(x), y(y), z(z) {}
    Vector3D(const Point3D& from, const Point3D& to) 
        : x(to.x - from.x), y(to.y - from.y), z(to.z - from.z) {}
    
    // Arithmetic operations
    Vector3D operator+(const Vector3D& other) const {
        return Vector3D(x + other.x, y + other.y, z + other.z);
    }
    
    Vector3D operator-(const Vector3D& other) const {
        return Vector3D(x - other.x, y - other.y, z - other.z);
    }
    
    Vector3D operator*(double scalar) const {
        return Vector3D(x * scalar, y * scalar, z * scalar);
    }
    
    Vector3D operator/(double scalar) const {
        return Vector3D(x / scalar, y / scalar, z / scalar);
    }
    
    Vector3D operator-() const {
        return Vector3D(-x, -y, -z);
    }
    
    // Compound assignment operators
    Vector3D& operator+=(const Vector3D& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }
    
    Vector3D& operator-=(const Vector3D& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }
    
    Vector3D& operator*=(double scalar) {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }
    
    // Comparison operators
    bool operator==(const Vector3D& other) const {
        const double epsilon = 1e-9;
        return std::abs(x - other.x) < epsilon &&
               std::abs(y - other.y) < epsilon &&
               std::abs(z - other.z) < epsilon;
    }
    
    bool operator!=(const Vector3D& other) const {
        return !(*this == other);
    }
    
    // Vector operations
    double length() const {
        return std::sqrt(x * x + y * y + z * z);
    }
    
    double lengthSquared() const {
        return x * x + y * y + z * z;
    }
    
    Vector3D normalized() const {
        double len = length();
        if (len < 1e-9) {
            return Vector3D(0, 0, 0);
        }
        return Vector3D(x / len, y / len, z / len);
    }
    
    void normalize() {
        double len = length();
        if (len > 1e-9) {
            x /= len;
            y /= len;
            z /= len;
        } else {
            x = y = z = 0.0;
        }
    }
    
    double dot(const Vector3D& other) const {
        return x * other.x + y * other.y + z * other.z;
    }
    
    Vector3D cross(const Vector3D& other) const {
        return Vector3D(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        );
    }
    
    double angleTo(const Vector3D& other) const {
        double dot_product = dot(other);
        double lengths = length() * other.length();
        if (lengths < 1e-9) {
            return 0.0;
        }
        double cos_angle = dot_product / lengths;
        // Clamp to avoid numerical errors
        cos_angle = std::max(-1.0, std::min(1.0, cos_angle));
        return std::acos(cos_angle);
    }
    
    bool isZero(double epsilon = 1e-9) const {
        return length() < epsilon;
    }
    
    bool isUnit(double epsilon = 1e-6) const {
        return std::abs(length() - 1.0) < epsilon;
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
    
    // Convert to Point3D (treating vector as position vector)
    Point3D toPoint() const {
        return Point3D(x, y, z);
    }
    
    // Static utility vectors
    static Vector3D zero() { return Vector3D(0, 0, 0); }
    static Vector3D unitX() { return Vector3D(1, 0, 0); }
    static Vector3D unitY() { return Vector3D(0, 1, 0); }
    static Vector3D unitZ() { return Vector3D(0, 0, 1); }
};

// Global operators
inline Vector3D operator*(double scalar, const Vector3D& vector) {
    return vector * scalar;
}

// Point + Vector = Point
inline Point3D operator+(const Point3D& point, const Vector3D& vector) {
    return Point3D(point.x + vector.x, point.y + vector.y, point.z + vector.z);
}

// Point - Vector = Point
inline Point3D operator-(const Point3D& point, const Vector3D& vector) {
    return Point3D(point.x - vector.x, point.y - vector.y, point.z - vector.z);
}

} // namespace Geometry
} // namespace KitchenCAD