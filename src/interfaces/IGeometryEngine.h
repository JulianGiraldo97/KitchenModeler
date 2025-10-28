#pragma once

#include "../geometry/Point3D.h"
#include "../geometry/Vector3D.h"
#include "../geometry/BoundingBox.h"
#include <memory>
#include <vector>

namespace KitchenCAD {

// Forward declarations
class Shape3D;
class Face;

/**
 * @brief Enumeration of boolean operations for geometry
 */
enum class BooleanOperation {
    Union,
    Difference,
    Intersection
};

/**
 * @brief Interface for geometric engine operations
 * 
 * This interface defines the contract for geometric operations including
 * primitive creation, boolean operations, and geometric calculations.
 * Implementations will typically wrap OpenCascade functionality.
 */
class IGeometryEngine {
public:
    virtual ~IGeometryEngine() = default;
    
    // Primitive creation
    virtual std::unique_ptr<Shape3D> createBox(const Geometry::Point3D& origin, 
                                               double width, double height, double depth) = 0;
    
    virtual std::unique_ptr<Shape3D> createCylinder(const Geometry::Point3D& center, 
                                                    double radius, double height) = 0;
    
    virtual std::unique_ptr<Shape3D> createSphere(const Geometry::Point3D& center, 
                                                  double radius) = 0;
    
    virtual std::unique_ptr<Shape3D> createCone(const Geometry::Point3D& base, 
                                                double baseRadius, double topRadius, double height) = 0;
    
    // Boolean operations
    virtual bool performBoolean(Shape3D& target, const Shape3D& tool, BooleanOperation op) = 0;
    
    // Geometric analysis
    virtual std::vector<Face> getFaces(const Shape3D& shape) = 0;
    virtual Geometry::BoundingBox getBoundingBox(const Shape3D& shape) = 0;
    virtual double getVolume(const Shape3D& shape) = 0;
    virtual double getSurfaceArea(const Shape3D& shape) = 0;
    
    // Geometric queries
    virtual bool intersects(const Shape3D& shape1, const Shape3D& shape2) = 0;
    virtual double distanceBetween(const Shape3D& shape1, const Shape3D& shape2) = 0;
    
    // Transformation
    virtual std::unique_ptr<Shape3D> transform(const Shape3D& shape, 
                                               const Geometry::Transform3D& transform) = 0;
    
    // Validation
    virtual bool isValidShape(const Shape3D& shape) = 0;
    virtual bool isClosed(const Shape3D& shape) = 0;
};

} // namespace KitchenCAD