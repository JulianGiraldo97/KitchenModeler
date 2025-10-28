#pragma once

#ifdef HAVE_OPENCASCADE

#include "../interfaces/IGeometryEngine.h"
#include "OCCTShape3D.h"

// OpenCascade includes
#include <TopoDS_Shape.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <gp_Pnt.hxx>
#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>

#include <memory>
#include <vector>

namespace KitchenCAD {

/**
 * @brief OpenCascade implementation of IGeometryEngine
 * 
 * This class provides geometric operations using OpenCascade Technology (OCCT).
 * It implements primitive creation, boolean operations, and geometric analysis
 * functions using OpenCascade's robust geometric kernel.
 */
class OpenCascadeGeometryEngine : public IGeometryEngine {
private:
    double tolerance_;  // Geometric tolerance for operations
    
public:
    /**
     * @brief Constructor with default tolerance
     */
    OpenCascadeGeometryEngine(double tolerance = 1e-7);
    
    /**
     * @brief Destructor
     */
    virtual ~OpenCascadeGeometryEngine() = default;
    
    // IGeometryEngine interface implementation
    
    // Primitive creation
    std::unique_ptr<Shape3D> createBox(const Geometry::Point3D& origin, 
                                       double width, double height, double depth) override;
    
    std::unique_ptr<Shape3D> createCylinder(const Geometry::Point3D& center, 
                                            double radius, double height) override;
    
    std::unique_ptr<Shape3D> createSphere(const Geometry::Point3D& center, 
                                          double radius) override;
    
    std::unique_ptr<Shape3D> createCone(const Geometry::Point3D& base, 
                                        double baseRadius, double topRadius, double height) override;
    
    // Boolean operations
    bool performBoolean(Shape3D& target, const Shape3D& tool, BooleanOperation op) override;
    
    // Geometric analysis
    std::vector<Face> getFaces(const Shape3D& shape) override;
    Geometry::BoundingBox getBoundingBox(const Shape3D& shape) override;
    double getVolume(const Shape3D& shape) override;
    double getSurfaceArea(const Shape3D& shape) override;
    
    // Geometric queries
    bool intersects(const Shape3D& shape1, const Shape3D& shape2) override;
    double distanceBetween(const Shape3D& shape1, const Shape3D& shape2) override;
    
    // Transformation
    std::unique_ptr<Shape3D> transform(const Shape3D& shape, 
                                       const Geometry::Transform3D& transform) override;
    
    // Validation
    bool isValidShape(const Shape3D& shape) override;
    bool isClosed(const Shape3D& shape) override;
    
    // OpenCascade-specific methods
    
    /**
     * @brief Get the geometric tolerance used by this engine
     */
    double getTolerance() const { return tolerance_; }
    
    /**
     * @brief Set the geometric tolerance
     */
    void setTolerance(double tolerance) { tolerance_ = tolerance; }
    
    /**
     * @brief Create a box from two corner points
     */
    std::unique_ptr<Shape3D> createBox(const Geometry::Point3D& corner1, 
                                       const Geometry::Point3D& corner2);
    
    /**
     * @brief Create a cylinder with axis direction
     */
    std::unique_ptr<Shape3D> createCylinder(const Geometry::Point3D& base, 
                                            const Geometry::Vector3D& axis,
                                            double radius, double height);
    
    /**
     * @brief Create a cone with axis direction
     */
    std::unique_ptr<Shape3D> createCone(const Geometry::Point3D& base,
                                        const Geometry::Vector3D& axis,
                                        double baseRadius, double topRadius, double height);
    
    /**
     * @brief Create a shape from OpenCascade TopoDS_Shape
     */
    std::unique_ptr<Shape3D> createFromOCCShape(const TopoDS_Shape& shape);
    
    /**
     * @brief Perform boolean union of multiple shapes
     */
    std::unique_ptr<Shape3D> performUnion(const std::vector<std::unique_ptr<Shape3D>>& shapes);
    
    /**
     * @brief Perform boolean intersection of multiple shapes
     */
    std::unique_ptr<Shape3D> performIntersection(const std::vector<std::unique_ptr<Shape3D>>& shapes);
    
    /**
     * @brief Check if two shapes are geometrically equivalent
     */
    bool areEquivalent(const Shape3D& shape1, const Shape3D& shape2);
    
    /**
     * @brief Get the center of mass of a shape
     */
    Geometry::Point3D getCenterOfMass(const Shape3D& shape);
    
    /**
     * @brief Get the moment of inertia of a shape
     */
    double getMomentOfInertia(const Shape3D& shape, const Geometry::Vector3D& axis);
    
    /**
     * @brief Mesh a shape for visualization
     */
    bool meshShape(Shape3D& shape, double linearDeflection = 0.1, double angularDeflection = 0.1);

private:
    /**
     * @brief Convert KitchenCAD Point3D to OpenCascade gp_Pnt
     */
    gp_Pnt toOCCPoint(const Geometry::Point3D& point) const;
    
    /**
     * @brief Convert KitchenCAD Vector3D to OpenCascade gp_Dir
     */
    gp_Dir toOCCDirection(const Geometry::Vector3D& vector) const;
    
    /**
     * @brief Convert OpenCascade gp_Pnt to KitchenCAD Point3D
     */
    Geometry::Point3D fromOCCPoint(const gp_Pnt& point) const;
    
    /**
     * @brief Create coordinate system from point and direction
     */
    gp_Ax2 createCoordinateSystem(const Geometry::Point3D& origin, 
                                  const Geometry::Vector3D& direction) const;
    
    /**
     * @brief Validate boolean operation inputs
     */
    bool validateBooleanInputs(const Shape3D& target, const Shape3D& tool) const;
    
    /**
     * @brief Get OCCTShape3D from Shape3D (with type checking)
     */
    const OCCTShape3D* getOCCTShape(const Shape3D& shape) const;
    
    /**
     * @brief Get mutable OCCTShape3D from Shape3D (with type checking)
     */
    OCCTShape3D* getOCCTShape(Shape3D& shape) const;
};

} // namespace KitchenCAD

#endif // HAVE_OPENCASCADE