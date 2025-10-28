#ifdef HAVE_OPENCASCADE

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../src/geometry/OpenCascadeGeometryEngine.h"
#include "../src/geometry/OCCTShape3D.h"

using namespace KitchenCAD;
using namespace KitchenCAD::Geometry;
using Catch::Approx;

TEST_CASE("OpenCascadeGeometryEngine - Basic Primitive Creation", "[opencascade][geometry][primitives]") {
    OpenCascadeGeometryEngine engine;
    
    SECTION("Create Box") {
        Point3D origin(0.0, 0.0, 0.0);
        auto box = engine.createBox(origin, 2.0, 3.0, 4.0);
        
        REQUIRE(box != nullptr);
        REQUIRE(box->isValid());
        REQUIRE(!box->isEmpty());
        REQUIRE(box->getType() == "Solid");
        
        // Check bounding box
        BoundingBox bbox = box->getBoundingBox();
        REQUIRE(bbox.min.x == Approx(0.0));
        REQUIRE(bbox.min.y == Approx(0.0));
        REQUIRE(bbox.min.z == Approx(0.0));
        REQUIRE(bbox.max.x == Approx(2.0));
        REQUIRE(bbox.max.y == Approx(3.0));
        REQUIRE(bbox.max.z == Approx(4.0));
        
        // Check volume
        REQUIRE(box->getVolume() == Approx(24.0));
        
        // Check surface area (2*(2*3 + 3*4 + 4*2) = 2*26 = 52)
        REQUIRE(box->getSurfaceArea() == Approx(52.0));
    }
    
    SECTION("Create Box from corners") {
        Point3D corner1(-1.0, -1.0, -1.0);
        Point3D corner2(1.0, 1.0, 1.0);
        auto box = engine.createBox(corner1, corner2);
        
        REQUIRE(box != nullptr);
        REQUIRE(box->isValid());
        REQUIRE(box->getVolume() == Approx(8.0));
    }
    
    SECTION("Create Cylinder") {
        Point3D center(0.0, 0.0, 0.0);
        auto cylinder = engine.createCylinder(center, 1.0, 2.0);
        
        REQUIRE(cylinder != nullptr);
        REQUIRE(cylinder->isValid());
        REQUIRE(!cylinder->isEmpty());
        REQUIRE(cylinder->getType() == "Solid");
        
        // Check volume (π * r² * h = π * 1² * 2 = 2π)
        REQUIRE(cylinder->getVolume() == Approx(2.0 * M_PI).margin(1e-6));
        
        // Check bounding box
        BoundingBox bbox = cylinder->getBoundingBox();
        REQUIRE(bbox.min.x == Approx(-1.0));
        REQUIRE(bbox.min.y == Approx(-1.0));
        REQUIRE(bbox.min.z == Approx(0.0));
        REQUIRE(bbox.max.x == Approx(1.0));
        REQUIRE(bbox.max.y == Approx(1.0));
        REQUIRE(bbox.max.z == Approx(2.0));
    }
    
    SECTION("Create Cylinder with custom axis") {
        Point3D base(0.0, 0.0, 0.0);
        Vector3D axis(1.0, 0.0, 0.0);  // X-axis
        auto cylinder = engine.createCylinder(base, axis, 1.0, 2.0);
        
        REQUIRE(cylinder != nullptr);
        REQUIRE(cylinder->isValid());
        REQUIRE(cylinder->getVolume() == Approx(2.0 * M_PI).margin(1e-6));
    }
    
    SECTION("Create Sphere") {
        Point3D center(0.0, 0.0, 0.0);
        auto sphere = engine.createSphere(center, 1.0);
        
        REQUIRE(sphere != nullptr);
        REQUIRE(sphere->isValid());
        REQUIRE(!sphere->isEmpty());
        REQUIRE(sphere->getType() == "Solid");
        
        // Check volume (4/3 * π * r³ = 4/3 * π * 1³ = 4π/3)
        REQUIRE(sphere->getVolume() == Approx(4.0 * M_PI / 3.0).margin(1e-6));
        
        // Check surface area (4 * π * r² = 4π)
        REQUIRE(sphere->getSurfaceArea() == Approx(4.0 * M_PI).margin(1e-6));
    }
    
    SECTION("Create Cone") {
        Point3D base(0.0, 0.0, 0.0);
        auto cone = engine.createCone(base, 1.0, 0.0, 2.0);  // Full cone
        
        REQUIRE(cone != nullptr);
        REQUIRE(cone->isValid());
        REQUIRE(!cone->isEmpty());
        REQUIRE(cone->getType() == "Solid");
        
        // Check volume (1/3 * π * r² * h = 1/3 * π * 1² * 2 = 2π/3)
        REQUIRE(cone->getVolume() == Approx(2.0 * M_PI / 3.0).margin(1e-6));
    }
    
    SECTION("Create Truncated Cone") {
        Point3D base(0.0, 0.0, 0.0);
        auto truncatedCone = engine.createCone(base, 2.0, 1.0, 3.0);
        
        REQUIRE(truncatedCone != nullptr);
        REQUIRE(truncatedCone->isValid());
        REQUIRE(!truncatedCone->isEmpty());
        
        // Volume should be greater than a cone with top radius 1.0
        // and less than a cylinder with radius 2.0
        double volume = truncatedCone->getVolume();
        REQUIRE(volume > M_PI * 1.0 * 1.0 * 3.0);  // > cylinder with r=1
        REQUIRE(volume < M_PI * 2.0 * 2.0 * 3.0);  // < cylinder with r=2
    }
}

TEST_CASE("OpenCascadeGeometryEngine - Invalid Primitive Parameters", "[opencascade][geometry][validation]") {
    OpenCascadeGeometryEngine engine;
    
    SECTION("Invalid box dimensions") {
        Point3D origin(0.0, 0.0, 0.0);
        
        auto invalidBox1 = engine.createBox(origin, 0.0, 1.0, 1.0);
        REQUIRE(invalidBox1 == nullptr);
        
        auto invalidBox2 = engine.createBox(origin, 1.0, -1.0, 1.0);
        REQUIRE(invalidBox2 == nullptr);
        
        auto invalidBox3 = engine.createBox(origin, 1.0, 1.0, 0.0);
        REQUIRE(invalidBox3 == nullptr);
    }
    
    SECTION("Invalid cylinder parameters") {
        Point3D center(0.0, 0.0, 0.0);
        
        auto invalidCylinder1 = engine.createCylinder(center, 0.0, 1.0);
        REQUIRE(invalidCylinder1 == nullptr);
        
        auto invalidCylinder2 = engine.createCylinder(center, 1.0, -1.0);
        REQUIRE(invalidCylinder2 == nullptr);
        
        // Zero-length axis
        Vector3D zeroAxis(0.0, 0.0, 0.0);
        auto invalidCylinder3 = engine.createCylinder(center, zeroAxis, 1.0, 1.0);
        REQUIRE(invalidCylinder3 == nullptr);
    }
    
    SECTION("Invalid sphere parameters") {
        Point3D center(0.0, 0.0, 0.0);
        
        auto invalidSphere1 = engine.createSphere(center, 0.0);
        REQUIRE(invalidSphere1 == nullptr);
        
        auto invalidSphere2 = engine.createSphere(center, -1.0);
        REQUIRE(invalidSphere2 == nullptr);
    }
    
    SECTION("Invalid cone parameters") {
        Point3D base(0.0, 0.0, 0.0);
        
        auto invalidCone1 = engine.createCone(base, -1.0, 1.0, 1.0);
        REQUIRE(invalidCone1 == nullptr);
        
        auto invalidCone2 = engine.createCone(base, 1.0, 1.0, 0.0);
        REQUIRE(invalidCone2 == nullptr);
        
        auto invalidCone3 = engine.createCone(base, 0.0, 0.0, 1.0);
        REQUIRE(invalidCone3 == nullptr);
    }
}

TEST_CASE("OpenCascadeGeometryEngine - Boolean Operations", "[opencascade][geometry][boolean]") {
    OpenCascadeGeometryEngine engine;
    
    SECTION("Union operation") {
        // Create two overlapping boxes
        auto box1 = engine.createBox(Point3D(0.0, 0.0, 0.0), 2.0, 2.0, 2.0);
        auto box2 = engine.createBox(Point3D(1.0, 0.0, 0.0), 2.0, 2.0, 2.0);
        
        REQUIRE(box1 != nullptr);
        REQUIRE(box2 != nullptr);
        
        double originalVolume1 = box1->getVolume();
        double originalVolume2 = box2->getVolume();
        
        bool success = engine.performBoolean(*box1, *box2, BooleanOperation::Union);
        REQUIRE(success);
        
        // Union volume should be less than sum of individual volumes due to overlap
        double unionVolume = box1->getVolume();
        REQUIRE(unionVolume < originalVolume1 + originalVolume2);
        REQUIRE(unionVolume > originalVolume1);  // Should be larger than original
        
        // Expected volume: 8 + 8 - 4 = 12 (overlap is 1x2x2 = 4)
        REQUIRE(unionVolume == Approx(12.0).margin(1e-6));
    }
    
    SECTION("Difference operation") {
        // Create a box and subtract a smaller box from it
        auto box1 = engine.createBox(Point3D(0.0, 0.0, 0.0), 2.0, 2.0, 2.0);
        auto box2 = engine.createBox(Point3D(0.5, 0.5, 0.5), 1.0, 1.0, 1.0);
        
        REQUIRE(box1 != nullptr);
        REQUIRE(box2 != nullptr);
        
        double originalVolume = box1->getVolume();
        double subtractedVolume = box2->getVolume();
        
        bool success = engine.performBoolean(*box1, *box2, BooleanOperation::Difference);
        REQUIRE(success);
        
        // Result volume should be original minus subtracted
        double resultVolume = box1->getVolume();
        REQUIRE(resultVolume == Approx(originalVolume - subtractedVolume).margin(1e-6));
        REQUIRE(resultVolume == Approx(7.0).margin(1e-6));  // 8 - 1 = 7
    }
    
    SECTION("Intersection operation") {
        // Create two overlapping boxes
        auto box1 = engine.createBox(Point3D(0.0, 0.0, 0.0), 2.0, 2.0, 2.0);
        auto box2 = engine.createBox(Point3D(1.0, 0.0, 0.0), 2.0, 2.0, 2.0);
        
        REQUIRE(box1 != nullptr);
        REQUIRE(box2 != nullptr);
        
        bool success = engine.performBoolean(*box1, *box2, BooleanOperation::Intersection);
        REQUIRE(success);
        
        // Intersection volume should be the overlap: 1x2x2 = 4
        double intersectionVolume = box1->getVolume();
        REQUIRE(intersectionVolume == Approx(4.0).margin(1e-6));
    }
    
    SECTION("Boolean operations with non-intersecting shapes") {
        // Create two separate boxes
        auto box1 = engine.createBox(Point3D(0.0, 0.0, 0.0), 1.0, 1.0, 1.0);
        auto box2 = engine.createBox(Point3D(5.0, 0.0, 0.0), 1.0, 1.0, 1.0);
        
        REQUIRE(box1 != nullptr);
        REQUIRE(box2 != nullptr);
        
        // Union should work
        bool unionSuccess = engine.performBoolean(*box1, *box2, BooleanOperation::Union);
        REQUIRE(unionSuccess);
        REQUIRE(box1->getVolume() == Approx(2.0).margin(1e-6));  // 1 + 1 = 2
        
        // Reset for intersection test
        box1 = engine.createBox(Point3D(0.0, 0.0, 0.0), 1.0, 1.0, 1.0);
        box2 = engine.createBox(Point3D(5.0, 0.0, 0.0), 1.0, 1.0, 1.0);
        
        // Intersection should result in empty shape
        bool intersectionSuccess = engine.performBoolean(*box1, *box2, BooleanOperation::Intersection);
        REQUIRE(intersectionSuccess);
        REQUIRE(box1->getVolume() == Approx(0.0).margin(1e-6));
    }
}

TEST_CASE("OpenCascadeGeometryEngine - Geometric Properties", "[opencascade][geometry][properties]") {
    OpenCascadeGeometryEngine engine;
    
    SECTION("Bounding box calculation") {
        auto box = engine.createBox(Point3D(1.0, 2.0, 3.0), 2.0, 3.0, 4.0);
        REQUIRE(box != nullptr);
        
        BoundingBox bbox = engine.getBoundingBox(*box);
        REQUIRE(bbox.min.x == Approx(1.0));
        REQUIRE(bbox.min.y == Approx(2.0));
        REQUIRE(bbox.min.z == Approx(3.0));
        REQUIRE(bbox.max.x == Approx(3.0));
        REQUIRE(bbox.max.y == Approx(5.0));
        REQUIRE(bbox.max.z == Approx(7.0));
    }
    
    SECTION("Volume calculation") {
        auto sphere = engine.createSphere(Point3D(0.0, 0.0, 0.0), 2.0);
        REQUIRE(sphere != nullptr);
        
        double volume = engine.getVolume(*sphere);
        double expectedVolume = 4.0 * M_PI * 8.0 / 3.0;  // 4/3 * π * r³
        REQUIRE(volume == Approx(expectedVolume).margin(1e-6));
    }
    
    SECTION("Surface area calculation") {
        auto sphere = engine.createSphere(Point3D(0.0, 0.0, 0.0), 2.0);
        REQUIRE(sphere != nullptr);
        
        double surfaceArea = engine.getSurfaceArea(*sphere);
        double expectedArea = 4.0 * M_PI * 4.0;  // 4 * π * r²
        REQUIRE(surfaceArea == Approx(expectedArea).margin(1e-6));
    }
    
    SECTION("Center of mass calculation") {
        auto box = engine.createBox(Point3D(0.0, 0.0, 0.0), 2.0, 4.0, 6.0);
        REQUIRE(box != nullptr);
        
        Point3D centerOfMass = engine.getCenterOfMass(*box);
        REQUIRE(centerOfMass.x == Approx(1.0));  // width/2
        REQUIRE(centerOfMass.y == Approx(2.0));  // height/2
        REQUIRE(centerOfMass.z == Approx(3.0));  // depth/2
    }
    
    SECTION("Face extraction") {
        auto box = engine.createBox(Point3D(0.0, 0.0, 0.0), 1.0, 1.0, 1.0);
        REQUIRE(box != nullptr);
        
        std::vector<Face> faces = engine.getFaces(*box);
        REQUIRE(faces.size() == 6);  // A box should have 6 faces
        
        // Check that all faces are valid
        for (const auto& face : faces) {
            REQUIRE(face.isValid());
            REQUIRE(face.getArea() > 0.0);
        }
    }
}

TEST_CASE("OpenCascadeGeometryEngine - Distance and Intersection", "[opencascade][geometry][queries]") {
    OpenCascadeGeometryEngine engine;
    
    SECTION("Distance between shapes") {
        // Create two separate boxes
        auto box1 = engine.createBox(Point3D(0.0, 0.0, 0.0), 1.0, 1.0, 1.0);
        auto box2 = engine.createBox(Point3D(3.0, 0.0, 0.0), 1.0, 1.0, 1.0);
        
        REQUIRE(box1 != nullptr);
        REQUIRE(box2 != nullptr);
        
        double distance = engine.distanceBetween(*box1, *box2);
        REQUIRE(distance == Approx(2.0).margin(1e-6));  // Gap between boxes
    }
    
    SECTION("Distance between touching shapes") {
        // Create two touching boxes
        auto box1 = engine.createBox(Point3D(0.0, 0.0, 0.0), 1.0, 1.0, 1.0);
        auto box2 = engine.createBox(Point3D(1.0, 0.0, 0.0), 1.0, 1.0, 1.0);
        
        REQUIRE(box1 != nullptr);
        REQUIRE(box2 != nullptr);
        
        double distance = engine.distanceBetween(*box1, *box2);
        REQUIRE(distance == Approx(0.0).margin(1e-6));
    }
    
    SECTION("Intersection detection") {
        // Create overlapping shapes
        auto box1 = engine.createBox(Point3D(0.0, 0.0, 0.0), 2.0, 2.0, 2.0);
        auto box2 = engine.createBox(Point3D(1.0, 1.0, 1.0), 2.0, 2.0, 2.0);
        
        REQUIRE(box1 != nullptr);
        REQUIRE(box2 != nullptr);
        
        bool intersects = engine.intersects(*box1, *box2);
        REQUIRE(intersects);
        
        // Create non-overlapping shapes
        auto box3 = engine.createBox(Point3D(5.0, 5.0, 5.0), 1.0, 1.0, 1.0);
        bool doesNotIntersect = engine.intersects(*box1, *box3);
        REQUIRE(!doesNotIntersect);
    }
}

TEST_CASE("OpenCascadeGeometryEngine - Transformations", "[opencascade][geometry][transform]") {
    OpenCascadeGeometryEngine engine;
    
    SECTION("Translation") {
        auto box = engine.createBox(Point3D(0.0, 0.0, 0.0), 1.0, 1.0, 1.0);
        REQUIRE(box != nullptr);
        
        Transform3D translation = Transform3D::fromTranslation(Vector3D(5.0, 3.0, 2.0));
        auto transformedBox = engine.transform(*box, translation);
        
        REQUIRE(transformedBox != nullptr);
        REQUIRE(transformedBox->isValid());
        
        BoundingBox bbox = transformedBox->getBoundingBox();
        REQUIRE(bbox.min.x == Approx(5.0));
        REQUIRE(bbox.min.y == Approx(3.0));
        REQUIRE(bbox.min.z == Approx(2.0));
        REQUIRE(bbox.max.x == Approx(6.0));
        REQUIRE(bbox.max.y == Approx(4.0));
        REQUIRE(bbox.max.z == Approx(3.0));
    }
    
    SECTION("Scaling") {
        auto box = engine.createBox(Point3D(0.0, 0.0, 0.0), 1.0, 1.0, 1.0);
        REQUIRE(box != nullptr);
        
        Transform3D scaling = Transform3D::fromScale(Vector3D(2.0, 2.0, 2.0));
        auto scaledBox = engine.transform(*box, scaling);
        
        REQUIRE(scaledBox != nullptr);
        REQUIRE(scaledBox->isValid());
        
        // Volume should be scaled by factor³ (2³ = 8)
        double scaledVolume = scaledBox->getVolume();
        REQUIRE(scaledVolume == Approx(8.0).margin(1e-6));
    }
    
    SECTION("Rotation") {
        auto box = engine.createBox(Point3D(0.0, 0.0, 0.0), 2.0, 1.0, 1.0);
        REQUIRE(box != nullptr);
        
        // 90-degree rotation around Z-axis
        Transform3D rotation = Transform3D::fromRotation(Vector3D(0.0, 0.0, M_PI / 2.0));
        auto rotatedBox = engine.transform(*box, rotation);
        
        REQUIRE(rotatedBox != nullptr);
        REQUIRE(rotatedBox->isValid());
        
        // Volume should remain the same
        REQUIRE(rotatedBox->getVolume() == Approx(2.0).margin(1e-6));
        
        // Bounding box should change due to rotation
        BoundingBox bbox = rotatedBox->getBoundingBox();
        // After 90° rotation, the 2x1x1 box becomes approximately 1x2x1
        REQUIRE(std::abs(bbox.width() - 1.0) < 0.1);
        REQUIRE(std::abs(bbox.height() - 2.0) < 0.1);
        REQUIRE(std::abs(bbox.depth() - 1.0) < 0.1);
    }
}

TEST_CASE("OpenCascadeGeometryEngine - Shape Validation", "[opencascade][geometry][validation]") {
    OpenCascadeGeometryEngine engine;
    
    SECTION("Valid shape validation") {
        auto box = engine.createBox(Point3D(0.0, 0.0, 0.0), 1.0, 1.0, 1.0);
        REQUIRE(box != nullptr);
        
        REQUIRE(engine.isValidShape(*box));
        REQUIRE(engine.isClosed(*box));
    }
    
    SECTION("Shape equivalence") {
        auto box1 = engine.createBox(Point3D(0.0, 0.0, 0.0), 1.0, 1.0, 1.0);
        auto box2 = engine.createBox(Point3D(0.0, 0.0, 0.0), 1.0, 1.0, 1.0);
        
        REQUIRE(box1 != nullptr);
        REQUIRE(box2 != nullptr);
        
        // Note: These are different instances, so they won't be equivalent
        // This tests the equivalence checking mechanism
        REQUIRE(!engine.areEquivalent(*box1, *box2));
        
        // Test with the same shape
        REQUIRE(engine.areEquivalent(*box1, *box1));
    }
}

TEST_CASE("OpenCascadeGeometryEngine - Multiple Shape Operations", "[opencascade][geometry][multiple]") {
    OpenCascadeGeometryEngine engine;
    
    SECTION("Union of multiple shapes") {
        std::vector<std::unique_ptr<Shape3D>> shapes;
        
        // Create three boxes
        shapes.push_back(engine.createBox(Point3D(0.0, 0.0, 0.0), 1.0, 1.0, 1.0));
        shapes.push_back(engine.createBox(Point3D(2.0, 0.0, 0.0), 1.0, 1.0, 1.0));
        shapes.push_back(engine.createBox(Point3D(4.0, 0.0, 0.0), 1.0, 1.0, 1.0));
        
        for (const auto& shape : shapes) {
            REQUIRE(shape != nullptr);
        }
        
        auto unionResult = engine.performUnion(shapes);
        REQUIRE(unionResult != nullptr);
        REQUIRE(unionResult->isValid());
        
        // Volume should be sum of individual volumes (no overlap)
        REQUIRE(unionResult->getVolume() == Approx(3.0).margin(1e-6));
    }
    
    SECTION("Intersection of multiple shapes") {
        std::vector<std::unique_ptr<Shape3D>> shapes;
        
        // Create three overlapping boxes
        shapes.push_back(engine.createBox(Point3D(0.0, 0.0, 0.0), 3.0, 3.0, 3.0));
        shapes.push_back(engine.createBox(Point3D(1.0, 0.0, 0.0), 3.0, 3.0, 3.0));
        shapes.push_back(engine.createBox(Point3D(0.0, 1.0, 0.0), 3.0, 3.0, 3.0));
        
        for (const auto& shape : shapes) {
            REQUIRE(shape != nullptr);
        }
        
        auto intersectionResult = engine.performIntersection(shapes);
        REQUIRE(intersectionResult != nullptr);
        REQUIRE(intersectionResult->isValid());
        
        // The intersection should be smaller than any individual shape
        REQUIRE(intersectionResult->getVolume() < 27.0);  // Less than 3³
        REQUIRE(intersectionResult->getVolume() > 0.0);   // But not empty
    }
}

#endif // HAVE_OPENCASCADE