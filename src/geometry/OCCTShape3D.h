#pragma once

#ifdef HAVE_OPENCASCADE

#include "../core/Shape3D.h"
#include "../geometry/Point3D.h"
#include "../geometry/Vector3D.h"
#include "../geometry/Transform3D.h"
#include "../geometry/BoundingBox.h"

// OpenCascade includes
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <TopExp_Explorer.hxx>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>
#include <GProp_GProps.hxx>
#include <BRepGProp.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepClass3d_SolidClassifier.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <gp_Trsf.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>

#include <memory>
#include <string>

namespace KitchenCAD {

/**
 * @brief OpenCascade implementation of Shape3D
 * 
 * This class wraps OpenCascade's TopoDS_Shape to provide the Shape3D interface.
 * It handles geometric operations, transformations, and property calculations
 * using OpenCascade's robust geometric kernel.
 */
class OCCTShape3D : public Shape3D {
private:
    TopoDS_Shape shape_;
    mutable bool boundingBoxCached_ = false;
    mutable Geometry::BoundingBox cachedBoundingBox_;
    mutable bool propertiesCached_ = false;
    mutable double cachedVolume_ = 0.0;
    mutable double cachedSurfaceArea_ = 0.0;

public:
    /**
     * @brief Construct from OpenCascade shape
     */
    explicit OCCTShape3D(const TopoDS_Shape& shape);
    
    /**
     * @brief Copy constructor
     */
    OCCTShape3D(const OCCTShape3D& other);
    
    /**
     * @brief Assignment operator
     */
    OCCTShape3D& operator=(const OCCTShape3D& other);
    
    /**
     * @brief Destructor
     */
    virtual ~OCCTShape3D() = default;
    
    // Shape3D interface implementation
    Geometry::BoundingBox getBoundingBox() const override;
    double getVolume() const override;
    double getSurfaceArea() const override;
    
    bool isValid() const override;
    bool isClosed() const override;
    bool isEmpty() const override;
    
    std::unique_ptr<Shape3D> transformed(const Geometry::Transform3D& transform) const override;
    void transform(const Geometry::Transform3D& transform) override;
    
    bool contains(const Geometry::Point3D& point) const override;
    double distanceTo(const Geometry::Point3D& point) const override;
    double distanceTo(const Shape3D& other) const override;
    
    bool intersects(const Shape3D& other) const override;
    bool intersects(const Geometry::BoundingBox& box) const override;
    
    std::unique_ptr<Shape3D> clone() const override;
    std::string getType() const override;
    
    bool serialize(const std::string& filePath) const override;
    bool deserialize(const std::string& filePath) override;
    
    // OpenCascade-specific methods
    
    /**
     * @brief Get the underlying OpenCascade shape
     */
    const TopoDS_Shape& getShape() const { return shape_; }
    
    /**
     * @brief Set the underlying OpenCascade shape
     */
    void setShape(const TopoDS_Shape& shape);
    
    /**
     * @brief Get all faces of the shape
     */
    std::vector<TopoDS_Face> getFaces() const;
    
    /**
     * @brief Get the number of faces
     */
    int getFaceCount() const;
    
    /**
     * @brief Get the number of edges
     */
    int getEdgeCount() const;
    
    /**
     * @brief Get the number of vertices
     */
    int getVertexCount() const;
    
    /**
     * @brief Check if the shape is a solid
     */
    bool isSolid() const;
    
    /**
     * @brief Check if the shape is a shell
     */
    bool isShell() const;
    
    /**
     * @brief Check if the shape is a face
     */
    bool isFace() const;
    
    /**
     * @brief Check if the shape is an edge
     */
    bool isEdge() const;
    
    /**
     * @brief Check if the shape is a vertex
     */
    bool isVertex() const;

private:
    /**
     * @brief Convert KitchenCAD Transform3D to OpenCascade gp_Trsf
     */
    gp_Trsf toOCCTransform(const Geometry::Transform3D& transform) const;
    
    /**
     * @brief Convert KitchenCAD Point3D to OpenCascade gp_Pnt
     */
    gp_Pnt toOCCPoint(const Geometry::Point3D& point) const;
    
    /**
     * @brief Convert OpenCascade gp_Pnt to KitchenCAD Point3D
     */
    Geometry::Point3D fromOCCPoint(const gp_Pnt& point) const;
    
    /**
     * @brief Calculate and cache geometric properties
     */
    void calculateProperties() const;
    
    /**
     * @brief Calculate and cache bounding box
     */
    void calculateBoundingBox() const;
    
    /**
     * @brief Clear cached values
     */
    void clearCache();
};

/**
 * @brief OpenCascade implementation of Face
 */
class OCCTFace : public Face {
private:
    TopoDS_Face face_;
    mutable bool propertiesCached_ = false;
    mutable double cachedArea_ = 0.0;
    mutable Geometry::Point3D cachedCentroid_;
    mutable Geometry::Vector3D cachedNormal_;
    mutable Geometry::BoundingBox cachedBoundingBox_;

public:
    /**
     * @brief Construct from OpenCascade face
     */
    explicit OCCTFace(const TopoDS_Face& face);
    
    // Face interface implementation
    double getArea() const override;
    Geometry::Point3D getCentroid() const override;
    Geometry::Vector3D getNormal() const override;
    Geometry::BoundingBox getBoundingBox() const override;
    
    bool isValid() const override;
    bool isPlanar() const override;
    
    bool contains(const Geometry::Point3D& point) const override;
    double distanceTo(const Geometry::Point3D& point) const override;
    
    std::vector<Geometry::Point3D> getVertices() const override;
    std::vector<std::vector<int>> getTriangles() const override;
    
    std::string getType() const override;
    
    /**
     * @brief Get the underlying OpenCascade face
     */
    const TopoDS_Face& getFace() const { return face_; }

private:
    /**
     * @brief Calculate and cache face properties
     */
    void calculateProperties() const;
    
    /**
     * @brief Clear cached values
     */
    void clearCache();
};

} // namespace KitchenCAD

#endif // HAVE_OPENCASCADE