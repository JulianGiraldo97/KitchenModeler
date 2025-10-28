#pragma once

#include "../geometry/BoundingBox.h"
#include "../geometry/Transform3D.h"
#include <memory>
#include <vector>

namespace KitchenCAD {

/**
 * @brief Abstract base class for 3D geometric shapes
 * 
 * This class provides the interface for all 3D geometric shapes
 * in the system, serving as a wrapper for underlying geometry
 * representations (e.g., OpenCascade TopoDS_Shape).
 */
class Shape3D {
public:
    virtual ~Shape3D() = default;
    
    // Geometric properties
    virtual Geometry::BoundingBox getBoundingBox() const = 0;
    virtual double getVolume() const = 0;
    virtual double getSurfaceArea() const = 0;
    
    // Validation
    virtual bool isValid() const = 0;
    virtual bool isClosed() const = 0;
    virtual bool isEmpty() const = 0;
    
    // Transformation
    virtual std::unique_ptr<Shape3D> transformed(const Geometry::Transform3D& transform) const = 0;
    virtual void transform(const Geometry::Transform3D& transform) = 0;
    
    // Geometric queries
    virtual bool contains(const Geometry::Point3D& point) const = 0;
    virtual double distanceTo(const Geometry::Point3D& point) const = 0;
    virtual double distanceTo(const Shape3D& other) const = 0;
    
    // Intersection tests
    virtual bool intersects(const Shape3D& other) const = 0;
    virtual bool intersects(const Geometry::BoundingBox& box) const = 0;
    
    // Cloning
    virtual std::unique_ptr<Shape3D> clone() const = 0;
    
    // Type information
    virtual std::string getType() const = 0;
    
    // Serialization support
    virtual bool serialize(const std::string& filePath) const = 0;
    virtual bool deserialize(const std::string& filePath) = 0;
};

/**
 * @brief Represents a face of a 3D shape
 */
class Face {
public:
    Face() = default;
    virtual ~Face() = default;
    
    // Geometric properties
    virtual double getArea() const = 0;
    virtual Geometry::Point3D getCentroid() const = 0;
    virtual Geometry::Vector3D getNormal() const = 0;
    virtual Geometry::BoundingBox getBoundingBox() const = 0;
    
    // Validation
    virtual bool isValid() const = 0;
    virtual bool isPlanar() const = 0;
    
    // Geometric queries
    virtual bool contains(const Geometry::Point3D& point) const = 0;
    virtual double distanceTo(const Geometry::Point3D& point) const = 0;
    
    // Mesh generation
    virtual std::vector<Geometry::Point3D> getVertices() const = 0;
    virtual std::vector<std::vector<int>> getTriangles() const = 0;
    
    // Type information
    virtual std::string getType() const = 0;
};

} // namespace KitchenCAD