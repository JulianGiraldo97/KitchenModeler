#pragma once

#include "Point3D.h"
#include "Vector3D.h"
#include <array>
#include <cmath>

namespace KitchenCAD {
namespace Geometry {

/**
 * @brief 4x4 transformation matrix for 3D operations
 * Matrix is stored in column-major order for OpenGL compatibility
 */
class Matrix4x4 {
private:
    std::array<double, 16> data_;
    
public:
    // Constructors
    Matrix4x4() {
        setIdentity();
    }
    
    Matrix4x4(const std::array<double, 16>& data) : data_(data) {}
    
    // Element access
    double& operator()(int row, int col) {
        return data_[col * 4 + row];  // Column-major order
    }
    
    const double& operator()(int row, int col) const {
        return data_[col * 4 + row];  // Column-major order
    }
    
    double* data() { return data_.data(); }
    const double* data() const { return data_.data(); }
    
    // Matrix operations
    Matrix4x4 operator*(const Matrix4x4& other) const {
        Matrix4x4 result;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                double sum = 0.0;
                for (int k = 0; k < 4; ++k) {
                    sum += (*this)(i, k) * other(k, j);
                }
                result(i, j) = sum;
            }
        }
        return result;
    }
    
    Matrix4x4& operator*=(const Matrix4x4& other) {
        *this = *this * other;
        return *this;
    }
    
    // Transform point (w = 1)
    Point3D transformPoint(const Point3D& point) const {
        double x = (*this)(0, 0) * point.x + (*this)(0, 1) * point.y + (*this)(0, 2) * point.z + (*this)(0, 3);
        double y = (*this)(1, 0) * point.x + (*this)(1, 1) * point.y + (*this)(1, 2) * point.z + (*this)(1, 3);
        double z = (*this)(2, 0) * point.x + (*this)(2, 1) * point.y + (*this)(2, 2) * point.z + (*this)(2, 3);
        double w = (*this)(3, 0) * point.x + (*this)(3, 1) * point.y + (*this)(3, 2) * point.z + (*this)(3, 3);
        
        if (std::abs(w) > 1e-9) {
            return Point3D(x / w, y / w, z / w);
        }
        return Point3D(x, y, z);
    }
    
    // Transform vector (w = 0)
    Vector3D transformVector(const Vector3D& vector) const {
        double x = (*this)(0, 0) * vector.x + (*this)(0, 1) * vector.y + (*this)(0, 2) * vector.z;
        double y = (*this)(1, 0) * vector.x + (*this)(1, 1) * vector.y + (*this)(1, 2) * vector.z;
        double z = (*this)(2, 0) * vector.x + (*this)(2, 1) * vector.y + (*this)(2, 2) * vector.z;
        return Vector3D(x, y, z);
    }
    
    // Matrix utilities
    void setIdentity() {
        data_.fill(0.0);
        (*this)(0, 0) = 1.0;
        (*this)(1, 1) = 1.0;
        (*this)(2, 2) = 1.0;
        (*this)(3, 3) = 1.0;
    }
    
    Matrix4x4 transposed() const {
        Matrix4x4 result;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                result(i, j) = (*this)(j, i);
            }
        }
        return result;
    }
    
    double determinant() const {
        // Calculate 4x4 determinant using cofactor expansion
        double det = 0.0;
        for (int i = 0; i < 4; ++i) {
            det += (*this)(0, i) * cofactor(0, i);
        }
        return det;
    }
    
    Matrix4x4 inverse() const {
        double det = determinant();
        if (std::abs(det) < 1e-9) {
            // Return identity if matrix is not invertible
            Matrix4x4 identity;
            return identity;
        }
        
        Matrix4x4 result;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                result(j, i) = cofactor(i, j) / det;  // Transposed cofactor matrix
            }
        }
        return result;
    }
    
    // Static factory methods
    static Matrix4x4 translation(const Vector3D& translation) {
        Matrix4x4 result;
        result(0, 3) = translation.x;
        result(1, 3) = translation.y;
        result(2, 3) = translation.z;
        return result;
    }
    
    static Matrix4x4 rotationX(double angle) {
        Matrix4x4 result;
        double c = std::cos(angle);
        double s = std::sin(angle);
        result(1, 1) = c;
        result(1, 2) = -s;
        result(2, 1) = s;
        result(2, 2) = c;
        return result;
    }
    
    static Matrix4x4 rotationY(double angle) {
        Matrix4x4 result;
        double c = std::cos(angle);
        double s = std::sin(angle);
        result(0, 0) = c;
        result(0, 2) = s;
        result(2, 0) = -s;
        result(2, 2) = c;
        return result;
    }
    
    static Matrix4x4 rotationZ(double angle) {
        Matrix4x4 result;
        double c = std::cos(angle);
        double s = std::sin(angle);
        result(0, 0) = c;
        result(0, 1) = -s;
        result(1, 0) = s;
        result(1, 1) = c;
        return result;
    }
    
    static Matrix4x4 scale(const Vector3D& scale) {
        Matrix4x4 result;
        result(0, 0) = scale.x;
        result(1, 1) = scale.y;
        result(2, 2) = scale.z;
        return result;
    }
    
    static Matrix4x4 lookAt(const Point3D& eye, const Point3D& target, const Vector3D& up) {
        Vector3D forward = Vector3D(eye, target).normalized();
        Vector3D right = forward.cross(up).normalized();
        Vector3D newUp = right.cross(forward);
        
        Matrix4x4 result;
        result(0, 0) = right.x;    result(0, 1) = right.y;    result(0, 2) = right.z;    result(0, 3) = -right.dot(Vector3D(Point3D(), eye));
        result(1, 0) = newUp.x;    result(1, 1) = newUp.y;    result(1, 2) = newUp.z;    result(1, 3) = -newUp.dot(Vector3D(Point3D(), eye));
        result(2, 0) = -forward.x; result(2, 1) = -forward.y; result(2, 2) = -forward.z; result(2, 3) = forward.dot(Vector3D(Point3D(), eye));
        result(3, 0) = 0.0;        result(3, 1) = 0.0;        result(3, 2) = 0.0;        result(3, 3) = 1.0;
        
        return result;
    }
    
    static Matrix4x4 perspective(double fovy, double aspect, double near, double far) {
        Matrix4x4 result;
        result.data_.fill(0.0);
        
        double f = 1.0 / std::tan(fovy * 0.5);
        result(0, 0) = f / aspect;
        result(1, 1) = f;
        result(2, 2) = (far + near) / (near - far);
        result(2, 3) = (2.0 * far * near) / (near - far);
        result(3, 2) = -1.0;
        
        return result;
    }
    
private:
    double cofactor(int row, int col) const {
        // Calculate 3x3 minor determinant
        std::array<double, 9> minor;
        int minorIndex = 0;
        
        for (int i = 0; i < 4; ++i) {
            if (i == row) continue;
            for (int j = 0; j < 4; ++j) {
                if (j == col) continue;
                minor[minorIndex++] = (*this)(i, j);
            }
        }
        
        double det3x3 = minor[0] * (minor[4] * minor[8] - minor[5] * minor[7]) -
                        minor[1] * (minor[3] * minor[8] - minor[5] * minor[6]) +
                        minor[2] * (minor[3] * minor[7] - minor[4] * minor[6]);
        
        // Apply checkerboard sign
        int sign = ((row + col) % 2 == 0) ? 1 : -1;
        return sign * det3x3;
    }
};

} // namespace Geometry
} // namespace KitchenCAD