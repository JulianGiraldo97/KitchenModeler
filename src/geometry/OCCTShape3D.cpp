#ifdef HAVE_OPENCASCADE

#include "OCCTShape3D.h"
#include "../utils/Logger.h"

// Additional OpenCascade includes
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <BRepClass3d_SolidClassifier.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <Poly_Triangulation.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <Poly_Array1OfTriangle.hxx>
#include <GeomLProp_SLProps.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <gp_Ax1.hxx>
#include <gp_Dir.hxx>
#include <Standard_Real.hxx>

#include <fstream>
#include <cmath>

namespace KitchenCAD {

// OCCTShape3D Implementation

OCCTShape3D::OCCTShape3D(const TopoDS_Shape& shape) : shape_(shape) {
    clearCache();
}

OCCTShape3D::OCCTShape3D(const OCCTShape3D& other) 
    : shape_(other.shape_)
    , boundingBoxCached_(other.boundingBoxCached_)
    , cachedBoundingBox_(other.cachedBoundingBox_)
    , propertiesCached_(other.propertiesCached_)
    , cachedVolume_(other.cachedVolume_)
    , cachedSurfaceArea_(other.cachedSurfaceArea_) {
}

OCCTShape3D& OCCTShape3D::operator=(const OCCTShape3D& other) {
    if (this != &other) {
        shape_ = other.shape_;
        boundingBoxCached_ = other.boundingBoxCached_;
        cachedBoundingBox_ = other.cachedBoundingBox_;
        propertiesCached_ = other.propertiesCached_;
        cachedVolume_ = other.cachedVolume_;
        cachedSurfaceArea_ = other.cachedSurfaceArea_;
    }
    return *this;
}

void OCCTShape3D::setShape(const TopoDS_Shape& shape) {
    shape_ = shape;
    clearCache();
}

Geometry::BoundingBox OCCTShape3D::getBoundingBox() const {
    if (!boundingBoxCached_) {
        calculateBoundingBox();
    }
    return cachedBoundingBox_;
}

double OCCTShape3D::getVolume() const {
    if (!propertiesCached_) {
        calculateProperties();
    }
    return cachedVolume_;
}

double OCCTShape3D::getSurfaceArea() const {
    if (!propertiesCached_) {
        calculateProperties();
    }
    return cachedSurfaceArea_;
}

bool OCCTShape3D::isValid() const {
    return !shape_.IsNull() && BRepTools::IsValid(shape_);
}

bool OCCTShape3D::isClosed() const {
    if (shape_.IsNull()) return false;
    
    try {
        return shape_.Closed();
    } catch (const Standard_Failure& e) {
        LOG_WARNING("Error checking if shape is closed: " + std::string(e.GetMessageString()));
        return false;
    }
}

bool OCCTShape3D::isEmpty() const {
    return shape_.IsNull();
}

std::unique_ptr<Shape3D> OCCTShape3D::transformed(const Geometry::Transform3D& transform) const {
    if (shape_.IsNull()) {
        return std::make_unique<OCCTShape3D>(TopoDS_Shape());
    }
    
    try {
        gp_Trsf trsf = toOCCTransform(transform);
        BRepBuilderAPI_Transform transformer(shape_, trsf);
        
        if (transformer.IsDone()) {
            return std::make_unique<OCCTShape3D>(transformer.Shape());
        } else {
            LOG_ERROR("Failed to transform shape");
            return std::make_unique<OCCTShape3D>(shape_);
        }
    } catch (const Standard_Failure& e) {
        LOG_ERROR("Error transforming shape: " + std::string(e.GetMessageString()));
        return std::make_unique<OCCTShape3D>(shape_);
    }
}

void OCCTShape3D::transform(const Geometry::Transform3D& transform) {
    if (shape_.IsNull()) return;
    
    try {
        gp_Trsf trsf = toOCCTransform(transform);
        BRepBuilderAPI_Transform transformer(shape_, trsf);
        
        if (transformer.IsDone()) {
            shape_ = transformer.Shape();
            clearCache();
        } else {
            LOG_ERROR("Failed to transform shape in place");
        }
    } catch (const Standard_Failure& e) {
        LOG_ERROR("Error transforming shape in place: " + std::string(e.GetMessageString()));
    }
}

bool OCCTShape3D::contains(const Geometry::Point3D& point) const {
    if (shape_.IsNull() || !isSolid()) return false;
    
    try {
        gp_Pnt occPoint = toOCCPoint(point);
        BRepClass3d_SolidClassifier classifier(shape_, occPoint, 1e-7);
        return classifier.State() == TopAbs_IN;
    } catch (const Standard_Failure& e) {
        LOG_WARNING("Error checking point containment: " + std::string(e.GetMessageString()));
        return false;
    }
}

double OCCTShape3D::distanceTo(const Geometry::Point3D& point) const {
    if (shape_.IsNull()) return std::numeric_limits<double>::infinity();
    
    try {
        gp_Pnt occPoint = toOCCPoint(point);
        
        // Create a vertex from the point
        BRep_Builder builder;
        TopoDS_Vertex vertex;
        builder.MakeVertex(vertex, occPoint, 1e-7);
        
        BRepExtrema_DistShapeShape distCalc(shape_, vertex);
        if (distCalc.IsDone() && distCalc.NbSolution() > 0) {
            return distCalc.Value();
        }
        
        return std::numeric_limits<double>::infinity();
    } catch (const Standard_Failure& e) {
        LOG_WARNING("Error calculating distance to point: " + std::string(e.GetMessageString()));
        return std::numeric_limits<double>::infinity();
    }
}

double OCCTShape3D::distanceTo(const Shape3D& other) const {
    const OCCTShape3D* otherOCCT = dynamic_cast<const OCCTShape3D*>(&other);
    if (!otherOCCT || shape_.IsNull() || otherOCCT->shape_.IsNull()) {
        return std::numeric_limits<double>::infinity();
    }
    
    try {
        BRepExtrema_DistShapeShape distCalc(shape_, otherOCCT->shape_);
        if (distCalc.IsDone() && distCalc.NbSolution() > 0) {
            return distCalc.Value();
        }
        
        return std::numeric_limits<double>::infinity();
    } catch (const Standard_Failure& e) {
        LOG_WARNING("Error calculating distance between shapes: " + std::string(e.GetMessageString()));
        return std::numeric_limits<double>::infinity();
    }
}

bool OCCTShape3D::intersects(const Shape3D& other) const {
    return distanceTo(other) <= 1e-7;
}

bool OCCTShape3D::intersects(const Geometry::BoundingBox& box) const {
    Geometry::BoundingBox myBox = getBoundingBox();
    return myBox.intersects(box);
}

std::unique_ptr<Shape3D> OCCTShape3D::clone() const {
    return std::make_unique<OCCTShape3D>(*this);
}

std::string OCCTShape3D::getType() const {
    if (shape_.IsNull()) return "Null";
    
    switch (shape_.ShapeType()) {
        case TopAbs_COMPOUND: return "Compound";
        case TopAbs_COMPSOLID: return "CompSolid";
        case TopAbs_SOLID: return "Solid";
        case TopAbs_SHELL: return "Shell";
        case TopAbs_FACE: return "Face";
        case TopAbs_WIRE: return "Wire";
        case TopAbs_EDGE: return "Edge";
        case TopAbs_VERTEX: return "Vertex";
        default: return "Unknown";
    }
}

bool OCCTShape3D::serialize(const std::string& filePath) const {
    if (shape_.IsNull()) return false;
    
    try {
        std::ofstream file(filePath);
        if (!file.is_open()) return false;
        
        BRepTools::Write(shape_, file);
        return true;
    } catch (const Standard_Failure& e) {
        LOG_ERROR("Error serializing shape: " + std::string(e.GetMessageString()));
        return false;
    }
}

bool OCCTShape3D::deserialize(const std::string& filePath) {
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) return false;
        
        BRep_Builder builder;
        TopoDS_Shape newShape;
        BRepTools::Read(newShape, file, builder);
        
        if (!newShape.IsNull()) {
            shape_ = newShape;
            clearCache();
            return true;
        }
        
        return false;
    } catch (const Standard_Failure& e) {
        LOG_ERROR("Error deserializing shape: " + std::string(e.GetMessageString()));
        return false;
    }
}

std::vector<TopoDS_Face> OCCTShape3D::getFaces() const {
    std::vector<TopoDS_Face> faces;
    
    if (shape_.IsNull()) return faces;
    
    try {
        TopExp_Explorer explorer(shape_, TopAbs_FACE);
        while (explorer.More()) {
            faces.push_back(TopoDS::Face(explorer.Current()));
            explorer.Next();
        }
    } catch (const Standard_Failure& e) {
        LOG_WARNING("Error getting faces: " + std::string(e.GetMessageString()));
    }
    
    return faces;
}

int OCCTShape3D::getFaceCount() const {
    if (shape_.IsNull()) return 0;
    
    try {
        TopTools_IndexedMapOfShape faceMap;
        TopExp::MapShapes(shape_, TopAbs_FACE, faceMap);
        return faceMap.Extent();
    } catch (const Standard_Failure& e) {
        LOG_WARNING("Error counting faces: " + std::string(e.GetMessageString()));
        return 0;
    }
}

int OCCTShape3D::getEdgeCount() const {
    if (shape_.IsNull()) return 0;
    
    try {
        TopTools_IndexedMapOfShape edgeMap;
        TopExp::MapShapes(shape_, TopAbs_EDGE, edgeMap);
        return edgeMap.Extent();
    } catch (const Standard_Failure& e) {
        LOG_WARNING("Error counting edges: " + std::string(e.GetMessageString()));
        return 0;
    }
}

int OCCTShape3D::getVertexCount() const {
    if (shape_.IsNull()) return 0;
    
    try {
        TopTools_IndexedMapOfShape vertexMap;
        TopExp::MapShapes(shape_, TopAbs_VERTEX, vertexMap);
        return vertexMap.Extent();
    } catch (const Standard_Failure& e) {
        LOG_WARNING("Error counting vertices: " + std::string(e.GetMessageString()));
        return 0;
    }
}

bool OCCTShape3D::isSolid() const {
    return !shape_.IsNull() && shape_.ShapeType() == TopAbs_SOLID;
}

bool OCCTShape3D::isShell() const {
    return !shape_.IsNull() && shape_.ShapeType() == TopAbs_SHELL;
}

bool OCCTShape3D::isFace() const {
    return !shape_.IsNull() && shape_.ShapeType() == TopAbs_FACE;
}

bool OCCTShape3D::isEdge() const {
    return !shape_.IsNull() && shape_.ShapeType() == TopAbs_EDGE;
}

bool OCCTShape3D::isVertex() const {
    return !shape_.IsNull() && shape_.ShapeType() == TopAbs_VERTEX;
}

gp_Trsf OCCTShape3D::toOCCTransform(const Geometry::Transform3D& transform) const {
    gp_Trsf trsf;
    
    // Apply translation
    if (!transform.translation.x == 0.0 || !transform.translation.y == 0.0 || !transform.translation.z == 0.0) {
        gp_Vec translation(transform.translation.x, transform.translation.y, transform.translation.z);
        trsf.SetTranslation(translation);
    }
    
    // Apply rotation (Euler angles)
    if (transform.rotation.length() > 1e-9) {
        gp_Trsf rotX, rotY, rotZ;
        
        if (std::abs(transform.rotation.x) > 1e-9) {
            gp_Ax1 axisX(gp_Pnt(0, 0, 0), gp_Dir(1, 0, 0));
            rotX.SetRotation(axisX, transform.rotation.x);
        }
        
        if (std::abs(transform.rotation.y) > 1e-9) {
            gp_Ax1 axisY(gp_Pnt(0, 0, 0), gp_Dir(0, 1, 0));
            rotY.SetRotation(axisY, transform.rotation.y);
        }
        
        if (std::abs(transform.rotation.z) > 1e-9) {
            gp_Ax1 axisZ(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1));
            rotZ.SetRotation(axisZ, transform.rotation.z);
        }
        
        // Combine rotations: Z * Y * X (standard Euler angle order)
        gp_Trsf rotation = rotZ * rotY * rotX;
        trsf = trsf * rotation;
    }
    
    // Apply scaling
    if (std::abs(transform.scale.x - 1.0) > 1e-9 || 
        std::abs(transform.scale.y - 1.0) > 1e-9 || 
        std::abs(transform.scale.z - 1.0) > 1e-9) {
        
        // OpenCascade only supports uniform scaling in gp_Trsf
        // For non-uniform scaling, we'd need to use more complex transformations
        double uniformScale = (transform.scale.x + transform.scale.y + transform.scale.z) / 3.0;
        if (std::abs(uniformScale - 1.0) > 1e-9) {
            gp_Trsf scaleTrsf;
            scaleTrsf.SetScale(gp_Pnt(0, 0, 0), uniformScale);
            trsf = trsf * scaleTrsf;
        }
    }
    
    return trsf;
}

gp_Pnt OCCTShape3D::toOCCPoint(const Geometry::Point3D& point) const {
    return gp_Pnt(point.x, point.y, point.z);
}

Geometry::Point3D OCCTShape3D::fromOCCPoint(const gp_Pnt& point) const {
    return Geometry::Point3D(point.X(), point.Y(), point.Z());
}

void OCCTShape3D::calculateProperties() const {
    if (shape_.IsNull()) {
        cachedVolume_ = 0.0;
        cachedSurfaceArea_ = 0.0;
        propertiesCached_ = true;
        return;
    }
    
    try {
        GProp_GProps volumeProps, surfaceProps;
        
        // Calculate volume (for solids)
        if (isSolid()) {
            BRepGProp::VolumeProperties(shape_, volumeProps);
            cachedVolume_ = volumeProps.Mass();
        } else {
            cachedVolume_ = 0.0;
        }
        
        // Calculate surface area
        BRepGProp::SurfaceProperties(shape_, surfaceProps);
        cachedSurfaceArea_ = surfaceProps.Mass();
        
        propertiesCached_ = true;
    } catch (const Standard_Failure& e) {
        LOG_WARNING("Error calculating shape properties: " + std::string(e.GetMessageString()));
        cachedVolume_ = 0.0;
        cachedSurfaceArea_ = 0.0;
        propertiesCached_ = true;
    }
}

void OCCTShape3D::calculateBoundingBox() const {
    if (shape_.IsNull()) {
        cachedBoundingBox_ = Geometry::BoundingBox();
        boundingBoxCached_ = true;
        return;
    }
    
    try {
        Bnd_Box box;
        BRepBndLib::Add(shape_, box);
        
        if (!box.IsVoid()) {
            Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
            box.Get(xMin, yMin, zMin, xMax, yMax, zMax);
            
            cachedBoundingBox_ = Geometry::BoundingBox(
                Geometry::Point3D(xMin, yMin, zMin),
                Geometry::Point3D(xMax, yMax, zMax)
            );
        } else {
            cachedBoundingBox_ = Geometry::BoundingBox();
        }
        
        boundingBoxCached_ = true;
    } catch (const Standard_Failure& e) {
        LOG_WARNING("Error calculating bounding box: " + std::string(e.GetMessageString()));
        cachedBoundingBox_ = Geometry::BoundingBox();
        boundingBoxCached_ = true;
    }
}

void OCCTShape3D::clearCache() {
    boundingBoxCached_ = false;
    propertiesCached_ = false;
    cachedVolume_ = 0.0;
    cachedSurfaceArea_ = 0.0;
}

// OCCTFace Implementation

OCCTFace::OCCTFace(const TopoDS_Face& face) : face_(face) {
    clearCache();
}

double OCCTFace::getArea() const {
    if (!propertiesCached_) {
        calculateProperties();
    }
    return cachedArea_;
}

Geometry::Point3D OCCTFace::getCentroid() const {
    if (!propertiesCached_) {
        calculateProperties();
    }
    return cachedCentroid_;
}

Geometry::Vector3D OCCTFace::getNormal() const {
    if (!propertiesCached_) {
        calculateProperties();
    }
    return cachedNormal_;
}

Geometry::BoundingBox OCCTFace::getBoundingBox() const {
    if (!propertiesCached_) {
        calculateProperties();
    }
    return cachedBoundingBox_;
}

bool OCCTFace::isValid() const {
    return !face_.IsNull() && BRepTools::IsValid(face_);
}

bool OCCTFace::isPlanar() const {
    if (face_.IsNull()) return false;
    
    try {
        BRepAdaptor_Surface surface(face_);
        return surface.GetType() == GeomAbs_Plane;
    } catch (const Standard_Failure& e) {
        LOG_WARNING("Error checking if face is planar: " + std::string(e.GetMessageString()));
        return false;
    }
}

bool OCCTFace::contains(const Geometry::Point3D& point) const {
    // This is a simplified implementation
    // For a complete implementation, you'd need to project the point onto the face
    // and check if it's within the face boundaries
    return getBoundingBox().contains(point);
}

double OCCTFace::distanceTo(const Geometry::Point3D& point) const {
    if (face_.IsNull()) return std::numeric_limits<double>::infinity();
    
    try {
        gp_Pnt occPoint(point.x, point.y, point.z);
        
        BRep_Builder builder;
        TopoDS_Vertex vertex;
        builder.MakeVertex(vertex, occPoint, 1e-7);
        
        BRepExtrema_DistShapeShape distCalc(face_, vertex);
        if (distCalc.IsDone() && distCalc.NbSolution() > 0) {
            return distCalc.Value();
        }
        
        return std::numeric_limits<double>::infinity();
    } catch (const Standard_Failure& e) {
        LOG_WARNING("Error calculating distance from face to point: " + std::string(e.GetMessageString()));
        return std::numeric_limits<double>::infinity();
    }
}

std::vector<Geometry::Point3D> OCCTFace::getVertices() const {
    std::vector<Geometry::Point3D> vertices;
    
    if (face_.IsNull()) return vertices;
    
    try {
        TopExp_Explorer explorer(face_, TopAbs_VERTEX);
        while (explorer.More()) {
            TopoDS_Vertex vertex = TopoDS::Vertex(explorer.Current());
            gp_Pnt point = BRep_Tool::Pnt(vertex);
            vertices.emplace_back(point.X(), point.Y(), point.Z());
            explorer.Next();
        }
    } catch (const Standard_Failure& e) {
        LOG_WARNING("Error getting face vertices: " + std::string(e.GetMessageString()));
    }
    
    return vertices;
}

std::vector<std::vector<int>> OCCTFace::getTriangles() const {
    std::vector<std::vector<int>> triangles;
    
    if (face_.IsNull()) return triangles;
    
    try {
        // Generate mesh if not already present
        BRepMesh_IncrementalMesh mesh(face_, 0.1);
        
        TopLoc_Location location;
        Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(face_, location);
        
        if (!triangulation.IsNull()) {
            const Poly_Array1OfTriangle& triangleArray = triangulation->Triangles();
            
            for (int i = triangleArray.Lower(); i <= triangleArray.Upper(); ++i) {
                const Poly_Triangle& triangle = triangleArray(i);
                Standard_Integer n1, n2, n3;
                triangle.Get(n1, n2, n3);
                
                // Convert to 0-based indexing
                triangles.push_back({n1 - 1, n2 - 1, n3 - 1});
            }
        }
    } catch (const Standard_Failure& e) {
        LOG_WARNING("Error getting face triangulation: " + std::string(e.GetMessageString()));
    }
    
    return triangles;
}

std::string OCCTFace::getType() const {
    if (face_.IsNull()) return "Null";
    
    try {
        BRepAdaptor_Surface surface(face_);
        switch (surface.GetType()) {
            case GeomAbs_Plane: return "Plane";
            case GeomAbs_Cylinder: return "Cylinder";
            case GeomAbs_Cone: return "Cone";
            case GeomAbs_Sphere: return "Sphere";
            case GeomAbs_Torus: return "Torus";
            case GeomAbs_BezierSurface: return "BezierSurface";
            case GeomAbs_BSplineSurface: return "BSplineSurface";
            case GeomAbs_SurfaceOfRevolution: return "SurfaceOfRevolution";
            case GeomAbs_SurfaceOfExtrusion: return "SurfaceOfExtrusion";
            case GeomAbs_OffsetSurface: return "OffsetSurface";
            default: return "OtherSurface";
        }
    } catch (const Standard_Failure& e) {
        LOG_WARNING("Error getting face type: " + std::string(e.GetMessageString()));
        return "Unknown";
    }
}

void OCCTFace::calculateProperties() const {
    if (face_.IsNull()) {
        cachedArea_ = 0.0;
        cachedCentroid_ = Geometry::Point3D();
        cachedNormal_ = Geometry::Vector3D();
        cachedBoundingBox_ = Geometry::BoundingBox();
        propertiesCached_ = true;
        return;
    }
    
    try {
        // Calculate area and centroid
        GProp_GProps props;
        BRepGProp::SurfaceProperties(face_, props);
        cachedArea_ = props.Mass();
        
        gp_Pnt centroid = props.CentreOfMass();
        cachedCentroid_ = Geometry::Point3D(centroid.X(), centroid.Y(), centroid.Z());
        
        // Calculate normal (at centroid for simplicity)
        BRepAdaptor_Surface surface(face_);
        Standard_Real u = (surface.FirstUParameter() + surface.LastUParameter()) * 0.5;
        Standard_Real v = (surface.FirstVParameter() + surface.LastVParameter()) * 0.5;
        
        gp_Pnt point;
        gp_Vec du, dv;
        surface.D1(u, v, point, du, dv);
        
        gp_Vec normal = du.Crossed(dv);
        if (normal.Magnitude() > 1e-9) {
            normal.Normalize();
            cachedNormal_ = Geometry::Vector3D(normal.X(), normal.Y(), normal.Z());
        } else {
            cachedNormal_ = Geometry::Vector3D(0, 0, 1); // Default normal
        }
        
        // Calculate bounding box
        Bnd_Box box;
        BRepBndLib::Add(face_, box);
        
        if (!box.IsVoid()) {
            Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
            box.Get(xMin, yMin, zMin, xMax, yMax, zMax);
            
            cachedBoundingBox_ = Geometry::BoundingBox(
                Geometry::Point3D(xMin, yMin, zMin),
                Geometry::Point3D(xMax, yMax, zMax)
            );
        } else {
            cachedBoundingBox_ = Geometry::BoundingBox();
        }
        
        propertiesCached_ = true;
    } catch (const Standard_Failure& e) {
        LOG_WARNING("Error calculating face properties: " + std::string(e.GetMessageString()));
        cachedArea_ = 0.0;
        cachedCentroid_ = Geometry::Point3D();
        cachedNormal_ = Geometry::Vector3D(0, 0, 1);
        cachedBoundingBox_ = Geometry::BoundingBox();
        propertiesCached_ = true;
    }
}

void OCCTFace::clearCache() {
    propertiesCached_ = false;
    cachedArea_ = 0.0;
    cachedCentroid_ = Geometry::Point3D();
    cachedNormal_ = Geometry::Vector3D();
    cachedBoundingBox_ = Geometry::BoundingBox();
}

} // namespace KitchenCAD

#endif // HAVE_OPENCASCADE