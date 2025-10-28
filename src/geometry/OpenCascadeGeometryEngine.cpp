#ifdef HAVE_OPENCASCADE

#include "OpenCascadeGeometryEngine.h"
#include "../utils/Logger.h"

// Additional OpenCascade includes
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepTools.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <gp_Vec.hxx>
#include <gp_Ax1.hxx>
#include <Standard_Failure.hxx>

#include <algorithm>
#include <cmath>

namespace KitchenCAD {

OpenCascadeGeometryEngine::OpenCascadeGeometryEngine(double tolerance) 
    : tolerance_(tolerance) {
    LOG_INFO("OpenCascade Geometry Engine initialized with tolerance: " + std::to_string(tolerance));
}

std::unique_ptr<Shape3D> OpenCascadeGeometryEngine::createBox(const Geometry::Point3D& origin, 
                                                              double width, double height, double depth) {
    if (width <= 0.0 || height <= 0.0 || depth <= 0.0) {
        LOG_ERROR("Invalid box dimensions: width=" + std::to_string(width) + 
                  ", height=" + std::to_string(height) + ", depth=" + std::to_string(depth));
        return nullptr;
    }
    
    try {
        gp_Pnt corner = toOCCPoint(origin);
        BRepPrimAPI_MakeBox boxMaker(corner, width, height, depth);
        
        if (boxMaker.IsDone()) {
            TopoDS_Shape box = boxMaker.Shape();
            return std::make_unique<OCCTShape3D>(box);
        } else {
            LOG_ERROR("Failed to create box");
            return nullptr;
        }
    } catch (const Standard_Failure& e) {
        LOG_ERROR("Error creating box: " + std::string(e.GetMessageString()));
        return nullptr;
    }
}

std::unique_ptr<Shape3D> OpenCascadeGeometryEngine::createBox(const Geometry::Point3D& corner1, 
                                                              const Geometry::Point3D& corner2) {
    try {
        gp_Pnt p1 = toOCCPoint(corner1);
        gp_Pnt p2 = toOCCPoint(corner2);
        
        BRepPrimAPI_MakeBox boxMaker(p1, p2);
        
        if (boxMaker.IsDone()) {
            TopoDS_Shape box = boxMaker.Shape();
            return std::make_unique<OCCTShape3D>(box);
        } else {
            LOG_ERROR("Failed to create box from corners");
            return nullptr;
        }
    } catch (const Standard_Failure& e) {
        LOG_ERROR("Error creating box from corners: " + std::string(e.GetMessageString()));
        return nullptr;
    }
}

std::unique_ptr<Shape3D> OpenCascadeGeometryEngine::createCylinder(const Geometry::Point3D& center, 
                                                                   double radius, double height) {
    if (radius <= 0.0 || height <= 0.0) {
        LOG_ERROR("Invalid cylinder dimensions: radius=" + std::to_string(radius) + 
                  ", height=" + std::to_string(height));
        return nullptr;
    }
    
    try {
        gp_Ax2 axis = createCoordinateSystem(center, Geometry::Vector3D(0, 0, 1));
        BRepPrimAPI_MakeCylinder cylinderMaker(axis, radius, height);
        
        if (cylinderMaker.IsDone()) {
            TopoDS_Shape cylinder = cylinderMaker.Shape();
            return std::make_unique<OCCTShape3D>(cylinder);
        } else {
            LOG_ERROR("Failed to create cylinder");
            return nullptr;
        }
    } catch (const Standard_Failure& e) {
        LOG_ERROR("Error creating cylinder: " + std::string(e.GetMessageString()));
        return nullptr;
    }
}

std::unique_ptr<Shape3D> OpenCascadeGeometryEngine::createCylinder(const Geometry::Point3D& base, 
                                                                   const Geometry::Vector3D& axis,
                                                                   double radius, double height) {
    if (radius <= 0.0 || height <= 0.0) {
        LOG_ERROR("Invalid cylinder dimensions: radius=" + std::to_string(radius) + 
                  ", height=" + std::to_string(height));
        return nullptr;
    }
    
    if (axis.length() < tolerance_) {
        LOG_ERROR("Invalid cylinder axis: zero length vector");
        return nullptr;
    }
    
    try {
        gp_Ax2 coordinateSystem = createCoordinateSystem(base, axis);
        BRepPrimAPI_MakeCylinder cylinderMaker(coordinateSystem, radius, height);
        
        if (cylinderMaker.IsDone()) {
            TopoDS_Shape cylinder = cylinderMaker.Shape();
            return std::make_unique<OCCTShape3D>(cylinder);
        } else {
            LOG_ERROR("Failed to create cylinder with custom axis");
            return nullptr;
        }
    } catch (const Standard_Failure& e) {
        LOG_ERROR("Error creating cylinder with custom axis: " + std::string(e.GetMessageString()));
        return nullptr;
    }
}

std::unique_ptr<Shape3D> OpenCascadeGeometryEngine::createSphere(const Geometry::Point3D& center, 
                                                                 double radius) {
    if (radius <= 0.0) {
        LOG_ERROR("Invalid sphere radius: " + std::to_string(radius));
        return nullptr;
    }
    
    try {
        gp_Pnt centerPoint = toOCCPoint(center);
        BRepPrimAPI_MakeSphere sphereMaker(centerPoint, radius);
        
        if (sphereMaker.IsDone()) {
            TopoDS_Shape sphere = sphereMaker.Shape();
            return std::make_unique<OCCTShape3D>(sphere);
        } else {
            LOG_ERROR("Failed to create sphere");
            return nullptr;
        }
    } catch (const Standard_Failure& e) {
        LOG_ERROR("Error creating sphere: " + std::string(e.GetMessageString()));
        return nullptr;
    }
}

std::unique_ptr<Shape3D> OpenCascadeGeometryEngine::createCone(const Geometry::Point3D& base, 
                                                               double baseRadius, double topRadius, double height) {
    if (baseRadius < 0.0 || topRadius < 0.0 || height <= 0.0) {
        LOG_ERROR("Invalid cone dimensions: baseRadius=" + std::to_string(baseRadius) + 
                  ", topRadius=" + std::to_string(topRadius) + ", height=" + std::to_string(height));
        return nullptr;
    }
    
    if (baseRadius < tolerance_ && topRadius < tolerance_) {
        LOG_ERROR("Invalid cone: both radii are zero");
        return nullptr;
    }
    
    try {
        gp_Ax2 axis = createCoordinateSystem(base, Geometry::Vector3D(0, 0, 1));
        BRepPrimAPI_MakeCone coneMaker(axis, baseRadius, topRadius, height);
        
        if (coneMaker.IsDone()) {
            TopoDS_Shape cone = coneMaker.Shape();
            return std::make_unique<OCCTShape3D>(cone);
        } else {
            LOG_ERROR("Failed to create cone");
            return nullptr;
        }
    } catch (const Standard_Failure& e) {
        LOG_ERROR("Error creating cone: " + std::string(e.GetMessageString()));
        return nullptr;
    }
}

std::unique_ptr<Shape3D> OpenCascadeGeometryEngine::createCone(const Geometry::Point3D& base,
                                                               const Geometry::Vector3D& axis,
                                                               double baseRadius, double topRadius, double height) {
    if (baseRadius < 0.0 || topRadius < 0.0 || height <= 0.0) {
        LOG_ERROR("Invalid cone dimensions: baseRadius=" + std::to_string(baseRadius) + 
                  ", topRadius=" + std::to_string(topRadius) + ", height=" + std::to_string(height));
        return nullptr;
    }
    
    if (baseRadius < tolerance_ && topRadius < tolerance_) {
        LOG_ERROR("Invalid cone: both radii are zero");
        return nullptr;
    }
    
    if (axis.length() < tolerance_) {
        LOG_ERROR("Invalid cone axis: zero length vector");
        return nullptr;
    }
    
    try {
        gp_Ax2 coordinateSystem = createCoordinateSystem(base, axis);
        BRepPrimAPI_MakeCone coneMaker(coordinateSystem, baseRadius, topRadius, height);
        
        if (coneMaker.IsDone()) {
            TopoDS_Shape cone = coneMaker.Shape();
            return std::make_unique<OCCTShape3D>(cone);
        } else {
            LOG_ERROR("Failed to create cone with custom axis");
            return nullptr;
        }
    } catch (const Standard_Failure& e) {
        LOG_ERROR("Error creating cone with custom axis: " + std::string(e.GetMessageString()));
        return nullptr;
    }
}

bool OpenCascadeGeometryEngine::performBoolean(Shape3D& target, const Shape3D& tool, BooleanOperation op) {
    if (!validateBooleanInputs(target, tool)) {
        return false;
    }
    
    OCCTShape3D* targetOCCT = getOCCTShape(target);
    const OCCTShape3D* toolOCCT = getOCCTShape(tool);
    
    if (!targetOCCT || !toolOCCT) {
        LOG_ERROR("Boolean operation requires OCCTShape3D objects");
        return false;
    }
    
    try {
        TopoDS_Shape result;
        bool success = false;
        
        switch (op) {
            case BooleanOperation::Union: {
                BRepAlgoAPI_Fuse fuseOp(targetOCCT->getShape(), toolOCCT->getShape());
                if (fuseOp.IsDone()) {
                    result = fuseOp.Shape();
                    success = true;
                }
                break;
            }
            case BooleanOperation::Difference: {
                BRepAlgoAPI_Cut cutOp(targetOCCT->getShape(), toolOCCT->getShape());
                if (cutOp.IsDone()) {
                    result = cutOp.Shape();
                    success = true;
                }
                break;
            }
            case BooleanOperation::Intersection: {
                BRepAlgoAPI_Common commonOp(targetOCCT->getShape(), toolOCCT->getShape());
                if (commonOp.IsDone()) {
                    result = commonOp.Shape();
                    success = true;
                }
                break;
            }
        }
        
        if (success && !result.IsNull()) {
            targetOCCT->setShape(result);
            return true;
        } else {
            LOG_ERROR("Boolean operation failed or produced null result");
            return false;
        }
    } catch (const Standard_Failure& e) {
        LOG_ERROR("Error performing boolean operation: " + std::string(e.GetMessageString()));
        return false;
    }
}

std::vector<Face> OpenCascadeGeometryEngine::getFaces(const Shape3D& shape) {
    std::vector<Face> faces;
    
    const OCCTShape3D* occShape = getOCCTShape(shape);
    if (!occShape) {
        LOG_ERROR("getFaces requires OCCTShape3D object");
        return faces;
    }
    
    try {
        std::vector<TopoDS_Face> occFaces = occShape->getFaces();
        faces.reserve(occFaces.size());
        
        for (const auto& occFace : occFaces) {
            faces.emplace_back(OCCTFace(occFace));
        }
    } catch (const Standard_Failure& e) {
        LOG_ERROR("Error getting faces: " + std::string(e.GetMessageString()));
    }
    
    return faces;
}

Geometry::BoundingBox OpenCascadeGeometryEngine::getBoundingBox(const Shape3D& shape) {
    return shape.getBoundingBox();
}

double OpenCascadeGeometryEngine::getVolume(const Shape3D& shape) {
    return shape.getVolume();
}

double OpenCascadeGeometryEngine::getSurfaceArea(const Shape3D& shape) {
    return shape.getSurfaceArea();
}

bool OpenCascadeGeometryEngine::intersects(const Shape3D& shape1, const Shape3D& shape2) {
    return shape1.intersects(shape2);
}

double OpenCascadeGeometryEngine::distanceBetween(const Shape3D& shape1, const Shape3D& shape2) {
    return shape1.distanceTo(shape2);
}

std::unique_ptr<Shape3D> OpenCascadeGeometryEngine::transform(const Shape3D& shape, 
                                                              const Geometry::Transform3D& transform) {
    return shape.transformed(transform);
}

bool OpenCascadeGeometryEngine::isValidShape(const Shape3D& shape) {
    return shape.isValid();
}

bool OpenCascadeGeometryEngine::isClosed(const Shape3D& shape) {
    return shape.isClosed();
}

std::unique_ptr<Shape3D> OpenCascadeGeometryEngine::createFromOCCShape(const TopoDS_Shape& shape) {
    return std::make_unique<OCCTShape3D>(shape);
}

std::unique_ptr<Shape3D> OpenCascadeGeometryEngine::performUnion(const std::vector<std::unique_ptr<Shape3D>>& shapes) {
    if (shapes.empty()) {
        LOG_ERROR("Cannot perform union on empty shape list");
        return nullptr;
    }
    
    if (shapes.size() == 1) {
        return shapes[0]->clone();
    }
    
    try {
        std::unique_ptr<Shape3D> result = shapes[0]->clone();
        
        for (size_t i = 1; i < shapes.size(); ++i) {
            if (!performBoolean(*result, *shapes[i], BooleanOperation::Union)) {
                LOG_ERROR("Failed to perform union at step " + std::to_string(i));
                return nullptr;
            }
        }
        
        return result;
    } catch (const Standard_Failure& e) {
        LOG_ERROR("Error performing multiple union: " + std::string(e.GetMessageString()));
        return nullptr;
    }
}

std::unique_ptr<Shape3D> OpenCascadeGeometryEngine::performIntersection(const std::vector<std::unique_ptr<Shape3D>>& shapes) {
    if (shapes.empty()) {
        LOG_ERROR("Cannot perform intersection on empty shape list");
        return nullptr;
    }
    
    if (shapes.size() == 1) {
        return shapes[0]->clone();
    }
    
    try {
        std::unique_ptr<Shape3D> result = shapes[0]->clone();
        
        for (size_t i = 1; i < shapes.size(); ++i) {
            if (!performBoolean(*result, *shapes[i], BooleanOperation::Intersection)) {
                LOG_ERROR("Failed to perform intersection at step " + std::to_string(i));
                return nullptr;
            }
        }
        
        return result;
    } catch (const Standard_Failure& e) {
        LOG_ERROR("Error performing multiple intersection: " + std::string(e.GetMessageString()));
        return nullptr;
    }
}

bool OpenCascadeGeometryEngine::areEquivalent(const Shape3D& shape1, const Shape3D& shape2) {
    const OCCTShape3D* occ1 = getOCCTShape(shape1);
    const OCCTShape3D* occ2 = getOCCTShape(shape2);
    
    if (!occ1 || !occ2) {
        return false;
    }
    
    try {
        return occ1->getShape().IsSame(occ2->getShape());
    } catch (const Standard_Failure& e) {
        LOG_WARNING("Error checking shape equivalence: " + std::string(e.GetMessageString()));
        return false;
    }
}

Geometry::Point3D OpenCascadeGeometryEngine::getCenterOfMass(const Shape3D& shape) {
    const OCCTShape3D* occShape = getOCCTShape(shape);
    if (!occShape) {
        LOG_ERROR("getCenterOfMass requires OCCTShape3D object");
        return Geometry::Point3D();
    }
    
    try {
        GProp_GProps props;
        
        if (occShape->isSolid()) {
            BRepGProp::VolumeProperties(occShape->getShape(), props);
        } else {
            BRepGProp::SurfaceProperties(occShape->getShape(), props);
        }
        
        gp_Pnt centerOfMass = props.CentreOfMass();
        return fromOCCPoint(centerOfMass);
    } catch (const Standard_Failure& e) {
        LOG_ERROR("Error calculating center of mass: " + std::string(e.GetMessageString()));
        return Geometry::Point3D();
    }
}

double OpenCascadeGeometryEngine::getMomentOfInertia(const Shape3D& shape, const Geometry::Vector3D& axis) {
    const OCCTShape3D* occShape = getOCCTShape(shape);
    if (!occShape) {
        LOG_ERROR("getMomentOfInertia requires OCCTShape3D object");
        return 0.0;
    }
    
    if (axis.length() < tolerance_) {
        LOG_ERROR("Invalid axis for moment of inertia calculation");
        return 0.0;
    }
    
    try {
        GProp_GProps props;
        
        if (occShape->isSolid()) {
            BRepGProp::VolumeProperties(occShape->getShape(), props);
        } else {
            BRepGProp::SurfaceProperties(occShape->getShape(), props);
        }
        
        gp_Pnt centerOfMass = props.CentreOfMass();
        gp_Dir axisDir = toOCCDirection(axis.normalized());
        gp_Ax1 inertiaAxis(centerOfMass, axisDir);
        
        return props.MomentOfInertia(inertiaAxis);
    } catch (const Standard_Failure& e) {
        LOG_ERROR("Error calculating moment of inertia: " + std::string(e.GetMessageString()));
        return 0.0;
    }
}

bool OpenCascadeGeometryEngine::meshShape(Shape3D& shape, double linearDeflection, double angularDeflection) {
    OCCTShape3D* occShape = getOCCTShape(shape);
    if (!occShape) {
        LOG_ERROR("meshShape requires OCCTShape3D object");
        return false;
    }
    
    try {
        BRepMesh_IncrementalMesh mesh(occShape->getShape(), linearDeflection, Standard_False, angularDeflection);
        return mesh.IsDone();
    } catch (const Standard_Failure& e) {
        LOG_ERROR("Error meshing shape: " + std::string(e.GetMessageString()));
        return false;
    }
}

// Private helper methods

gp_Pnt OpenCascadeGeometryEngine::toOCCPoint(const Geometry::Point3D& point) const {
    return gp_Pnt(point.x, point.y, point.z);
}

gp_Dir OpenCascadeGeometryEngine::toOCCDirection(const Geometry::Vector3D& vector) const {
    Geometry::Vector3D normalized = vector.normalized();
    return gp_Dir(normalized.x, normalized.y, normalized.z);
}

Geometry::Point3D OpenCascadeGeometryEngine::fromOCCPoint(const gp_Pnt& point) const {
    return Geometry::Point3D(point.X(), point.Y(), point.Z());
}

gp_Ax2 OpenCascadeGeometryEngine::createCoordinateSystem(const Geometry::Point3D& origin, 
                                                         const Geometry::Vector3D& direction) const {
    gp_Pnt originPoint = toOCCPoint(origin);
    gp_Dir zDirection = toOCCDirection(direction);
    
    // Create a coordinate system with the given origin and Z direction
    // OpenCascade will automatically determine appropriate X and Y directions
    return gp_Ax2(originPoint, zDirection);
}

bool OpenCascadeGeometryEngine::validateBooleanInputs(const Shape3D& target, const Shape3D& tool) const {
    if (!target.isValid()) {
        LOG_ERROR("Boolean operation target shape is invalid");
        return false;
    }
    
    if (!tool.isValid()) {
        LOG_ERROR("Boolean operation tool shape is invalid");
        return false;
    }
    
    if (target.isEmpty()) {
        LOG_ERROR("Boolean operation target shape is empty");
        return false;
    }
    
    if (tool.isEmpty()) {
        LOG_ERROR("Boolean operation tool shape is empty");
        return false;
    }
    
    return true;
}

const OCCTShape3D* OpenCascadeGeometryEngine::getOCCTShape(const Shape3D& shape) const {
    return dynamic_cast<const OCCTShape3D*>(&shape);
}

OCCTShape3D* OpenCascadeGeometryEngine::getOCCTShape(Shape3D& shape) const {
    return dynamic_cast<OCCTShape3D*>(&shape);
}

} // namespace KitchenCAD

#endif // HAVE_OPENCASCADE