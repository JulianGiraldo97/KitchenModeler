#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../src/geometry/Geometry.h"

using namespace KitchenCAD::Geometry;
using Catch::Approx;

TEST_CASE("Point3D basic operations", "[geometry][point3d]") {
    Point3D p1(1.0, 2.0, 3.0);
    Point3D p2(4.0, 5.0, 6.0);
    
    SECTION("Construction and access") {
        REQUIRE(p1.x == Approx(1.0));
        REQUIRE(p1.y == Approx(2.0));
        REQUIRE(p1.z == Approx(3.0));
        
        REQUIRE(p1[0] == Approx(1.0));
        REQUIRE(p1[1] == Approx(2.0));
        REQUIRE(p1[2] == Approx(3.0));
    }
    
    SECTION("Arithmetic operations") {
        Point3D sum = p1 + p2;
        REQUIRE(sum.x == Approx(5.0));
        REQUIRE(sum.y == Approx(7.0));
        REQUIRE(sum.z == Approx(9.0));
        
        Vector3D diff = p2 - p1;
        REQUIRE(diff.x == Approx(3.0));
        REQUIRE(diff.y == Approx(3.0));
        REQUIRE(diff.z == Approx(3.0));
        
        Point3D scaled = p1 * 2.0;
        REQUIRE(scaled.x == Approx(2.0));
        REQUIRE(scaled.y == Approx(4.0));
        REQUIRE(scaled.z == Approx(6.0));
    }
    
    SECTION("Distance calculations") {
        double distance = p1.distanceTo(p2);
        REQUIRE(distance == Approx(5.196152422706632));
        
        double distanceSquared = p1.distanceSquaredTo(p2);
        REQUIRE(distanceSquared == Approx(27.0));
    }
    
    SECTION("Equality comparison") {
        Point3D p3(1.0, 2.0, 3.0);
        REQUIRE(p1 == p3);
        REQUIRE(p1 != p2);
    }
}

TEST_CASE("Vector3D basic operations", "[geometry][vector3d]") {
    Vector3D v1(1.0, 0.0, 0.0);
    Vector3D v2(0.0, 1.0, 0.0);
    Vector3D v3(3.0, 4.0, 0.0);
    
    SECTION("Construction and access") {
        REQUIRE(v1.x == Approx(1.0));
        REQUIRE(v1.y == Approx(0.0));
        REQUIRE(v1.z == Approx(0.0));
    }
    
    SECTION("Length calculations") {
        REQUIRE(v1.length() == Approx(1.0));
        REQUIRE(v3.length() == Approx(5.0));
        REQUIRE(v3.lengthSquared() == Approx(25.0));
    }
    
    SECTION("Normalization") {
        Vector3D normalized = v3.normalized();
        REQUIRE(normalized.length() == Approx(1.0));
        REQUIRE(normalized.x == Approx(0.6));
        REQUIRE(normalized.y == Approx(0.8));
        REQUIRE(normalized.z == Approx(0.0));
    }
    
    SECTION("Dot product") {
        double dot = v1.dot(v2);
        REQUIRE(dot == Approx(0.0));
        
        Vector3D v4(1.0, 1.0, 0.0);
        double dot2 = v1.dot(v4);
        REQUIRE(dot2 == Approx(1.0));
    }
    
    SECTION("Cross product") {
        Vector3D cross = v1.cross(v2);
        REQUIRE(cross.x == Approx(0.0));
        REQUIRE(cross.y == Approx(0.0));
        REQUIRE(cross.z == Approx(1.0));
    }
    
    SECTION("Angle calculations") {
        double angle = v1.angleTo(v2);
        REQUIRE(angle == Approx(GeometryUtils::HALF_PI));
    }
    
    SECTION("Static utility vectors") {
        Vector3D zero = Vector3D::zero();
        REQUIRE(zero.isZero());
        
        Vector3D unitX = Vector3D::unitX();
        REQUIRE(unitX.isUnit());
        REQUIRE(unitX.x == Approx(1.0));
        REQUIRE(unitX.y == Approx(0.0));
        REQUIRE(unitX.z == Approx(0.0));
    }
}

TEST_CASE("Matrix4x4 operations", "[geometry][matrix]") {
    Matrix4x4 identity;
    
    SECTION("Identity matrix") {
        REQUIRE(identity(0, 0) == Approx(1.0));
        REQUIRE(identity(1, 1) == Approx(1.0));
        REQUIRE(identity(2, 2) == Approx(1.0));
        REQUIRE(identity(3, 3) == Approx(1.0));
        REQUIRE(identity(0, 1) == Approx(0.0));
        REQUIRE(identity(1, 0) == Approx(0.0));
    }
    
    SECTION("Translation matrix") {
        Matrix4x4 translation = Matrix4x4::translation(Vector3D(5.0, 10.0, 15.0));
        Point3D point(1.0, 1.0, 1.0);
        Point3D transformed = translation.transformPoint(point);
        
        REQUIRE(transformed.x == Approx(6.0));
        REQUIRE(transformed.y == Approx(11.0));
        REQUIRE(transformed.z == Approx(16.0));
    }
    
    SECTION("Rotation matrix") {
        Matrix4x4 rotationZ = Matrix4x4::rotationZ(GeometryUtils::HALF_PI);
        Point3D point(1.0, 0.0, 0.0);
        Point3D rotated = rotationZ.transformPoint(point);
        
        REQUIRE(rotated.x == Approx(0.0).margin(1e-10));
        REQUIRE(rotated.y == Approx(1.0));
        REQUIRE(rotated.z == Approx(0.0));
    }
    
    SECTION("Scale matrix") {
        Matrix4x4 scale = Matrix4x4::scale(Vector3D(2.0, 3.0, 4.0));
        Point3D point(1.0, 1.0, 1.0);
        Point3D scaled = scale.transformPoint(point);
        
        REQUIRE(scaled.x == Approx(2.0));
        REQUIRE(scaled.y == Approx(3.0));
        REQUIRE(scaled.z == Approx(4.0));
    }
    
    SECTION("Matrix multiplication") {
        Matrix4x4 translation = Matrix4x4::translation(Vector3D(1.0, 2.0, 3.0));
        Matrix4x4 scale = Matrix4x4::scale(Vector3D(2.0, 2.0, 2.0));
        Matrix4x4 combined = translation * scale;
        
        Point3D point(1.0, 1.0, 1.0);
        Point3D result = combined.transformPoint(point);
        
        REQUIRE(result.x == Approx(3.0));
        REQUIRE(result.y == Approx(4.0));
        REQUIRE(result.z == Approx(5.0));
    }
}

TEST_CASE("Transform3D operations", "[geometry][transform]") {
    Transform3D transform;
    
    SECTION("Identity transform") {
        REQUIRE(transform.isIdentity());
        
        Point3D point(1.0, 2.0, 3.0);
        Point3D transformed = transform.transformPoint(point);
        REQUIRE(transformed == point);
    }
    
    SECTION("Translation") {
        Transform3D translation = Transform3D::fromTranslation(Vector3D(5.0, 10.0, 15.0));
        Point3D point(1.0, 1.0, 1.0);
        Point3D transformed = translation.transformPoint(point);
        
        REQUIRE(transformed.x == Approx(6.0));
        REQUIRE(transformed.y == Approx(11.0));
        REQUIRE(transformed.z == Approx(16.0));
    }
    
    SECTION("Rotation") {
        Transform3D rotation = Transform3D::fromRotation(Vector3D(0.0, 0.0, GeometryUtils::HALF_PI));
        Point3D point(1.0, 0.0, 0.0);
        Point3D rotated = rotation.transformPoint(point);
        
        REQUIRE(rotated.x == Approx(0.0).margin(1e-10));
        REQUIRE(rotated.y == Approx(1.0));
        REQUIRE(rotated.z == Approx(0.0));
    }
    
    SECTION("Scale") {
        Transform3D scale = Transform3D::fromScale(Vector3D(2.0, 3.0, 4.0));
        Point3D point(1.0, 1.0, 1.0);
        Point3D scaled = scale.transformPoint(point);
        
        REQUIRE(scaled.x == Approx(2.0));
        REQUIRE(scaled.y == Approx(3.0));
        REQUIRE(scaled.z == Approx(4.0));
    }
    
    SECTION("Interpolation") {
        Transform3D t1(Point3D(0.0, 0.0, 0.0));
        Transform3D t2(Point3D(10.0, 20.0, 30.0));
        
        Transform3D lerped = t1.lerp(t2, 0.5);
        REQUIRE(lerped.translation.x == Approx(5.0));
        REQUIRE(lerped.translation.y == Approx(10.0));
        REQUIRE(lerped.translation.z == Approx(15.0));
    }
}

TEST_CASE("BoundingBox operations", "[geometry][boundingbox]") {
    BoundingBox bbox(Point3D(-1.0, -2.0, -3.0), Point3D(1.0, 2.0, 3.0));
    
    SECTION("Basic properties") {
        REQUIRE(bbox.isValid());
        REQUIRE(!bbox.isEmpty());
        
        Point3D center = bbox.center();
        REQUIRE(center.x == Approx(0.0));
        REQUIRE(center.y == Approx(0.0));
        REQUIRE(center.z == Approx(0.0));
        
        Vector3D size = bbox.size();
        REQUIRE(size.x == Approx(2.0));
        REQUIRE(size.y == Approx(4.0));
        REQUIRE(size.z == Approx(6.0));
        
        REQUIRE(bbox.volume() == Approx(48.0));
    }
    
    SECTION("Containment tests") {
        REQUIRE(bbox.contains(Point3D(0.0, 0.0, 0.0)));
        REQUIRE(bbox.contains(Point3D(1.0, 2.0, 3.0)));
        REQUIRE(!bbox.contains(Point3D(2.0, 0.0, 0.0)));
    }
    
    SECTION("Intersection tests") {
        BoundingBox other(Point3D(0.0, 0.0, 0.0), Point3D(2.0, 3.0, 4.0));
        REQUIRE(bbox.intersects(other));
        
        BoundingBox far(Point3D(10.0, 10.0, 10.0), Point3D(20.0, 20.0, 20.0));
        REQUIRE(!bbox.intersects(far));
    }
    
    SECTION("Expansion") {
        BoundingBox expanded = bbox.expanded(1.0);
        REQUIRE(expanded.min.x == Approx(-2.0));
        REQUIRE(expanded.max.x == Approx(2.0));
        REQUIRE(expanded.min.y == Approx(-3.0));
        REQUIRE(expanded.max.y == Approx(3.0));
    }
    
    SECTION("Distance calculations") {
        Point3D outsidePoint(5.0, 0.0, 0.0);
        double distance = bbox.distanceTo(outsidePoint);
        REQUIRE(distance == Approx(4.0));
        
        Point3D insidePoint(0.0, 0.0, 0.0);
        double insideDistance = bbox.distanceTo(insidePoint);
        REQUIRE(insideDistance == Approx(0.0));
    }
}

TEST_CASE("GeometryUtils operations", "[geometry][utils]") {
    SECTION("Angle conversions") {
        double radians = GeometryUtils::degreesToRadians(90.0);
        REQUIRE(radians == Approx(GeometryUtils::HALF_PI));
        
        double degrees = GeometryUtils::radiansToDegrees(GeometryUtils::PI);
        REQUIRE(degrees == Approx(180.0));
    }
    
    SECTION("Floating point comparisons") {
        REQUIRE(GeometryUtils::isEqual(1.0, 1.0000000001));
        REQUIRE(!GeometryUtils::isEqual(1.0, 1.1));
        REQUIRE(GeometryUtils::isZero(0.0000000001));
    }
    
    SECTION("Clamping") {
        REQUIRE(GeometryUtils::clamp(5.0, 0.0, 10.0) == Approx(5.0));
        REQUIRE(GeometryUtils::clamp(-5.0, 0.0, 10.0) == Approx(0.0));
        REQUIRE(GeometryUtils::clamp(15.0, 0.0, 10.0) == Approx(10.0));
    }
    
    SECTION("Interpolation") {
        double lerped = GeometryUtils::lerp(0.0, 10.0, 0.5);
        REQUIRE(lerped == Approx(5.0));
        
        Point3D p1(0.0, 0.0, 0.0);
        Point3D p2(10.0, 20.0, 30.0);
        Point3D lerpedPoint = GeometryUtils::lerp(p1, p2, 0.3);
        REQUIRE(lerpedPoint.x == Approx(3.0));
        REQUIRE(lerpedPoint.y == Approx(6.0));
        REQUIRE(lerpedPoint.z == Approx(9.0));
    }
    
    SECTION("Grid snapping") {
        Point3D point(1.23, 4.56, 7.89);
        Point3D snapped = GeometryUtils::snapToGrid(point, 0.5);
        REQUIRE(snapped.x == Approx(1.0));
        REQUIRE(snapped.y == Approx(4.5));
        REQUIRE(snapped.z == Approx(8.0));
    }
    
    SECTION("Distance calculations") {
        Point3D point(5.0, 0.0, 0.0);
        Point3D lineStart(0.0, 0.0, 0.0);
        Point3D lineEnd(10.0, 0.0, 0.0);
        
        double distance = GeometryUtils::pointToLineDistance(point, lineStart, lineEnd);
        REQUIRE(distance == Approx(0.0));
        
        Point3D offPoint(5.0, 3.0, 0.0);
        double offDistance = GeometryUtils::pointToLineDistance(offPoint, lineStart, lineEnd);
        REQUIRE(offDistance == Approx(3.0));
    }
}

TEST_CASE("Dimensions3D operations", "[geometry][dimensions]") {
    Dimensions3D dims(2.0, 3.0, 4.0);
    
    SECTION("Basic properties") {
        REQUIRE(dims.width == Approx(2.0));
        REQUIRE(dims.height == Approx(3.0));
        REQUIRE(dims.depth == Approx(4.0));
        REQUIRE(dims.volume() == Approx(24.0));
        REQUIRE(dims.isValid());
    }
    
    SECTION("Conversions") {
        Vector3D vec = dims.toVector();
        REQUIRE(vec.x == Approx(2.0));
        REQUIRE(vec.y == Approx(3.0));
        REQUIRE(vec.z == Approx(4.0));
        
        BoundingBox bbox = dims.toBoundingBox();
        Vector3D size = bbox.size();
        REQUIRE(size.x == Approx(2.0));
        REQUIRE(size.y == Approx(3.0));
        REQUIRE(size.z == Approx(4.0));
    }
}

TEST_CASE("Color operations", "[geometry][color]") {
    Color red = Color::red();
    Color blue = Color::blue();
    
    SECTION("Basic properties") {
        REQUIRE(red.r == Approx(1.0f));
        REQUIRE(red.g == Approx(0.0f));
        REQUIRE(red.b == Approx(0.0f));
        REQUIRE(red.a == Approx(1.0f));
    }
    
    SECTION("Color operations") {
        Color purple = red + blue;
        REQUIRE(purple.r == Approx(1.0f));
        REQUIRE(purple.g == Approx(0.0f));
        REQUIRE(purple.b == Approx(1.0f));
        
        Color darkRed = red * 0.5f;
        REQUIRE(darkRed.r == Approx(0.5f));
        REQUIRE(darkRed.g == Approx(0.0f));
        REQUIRE(darkRed.b == Approx(0.0f));
    }
    
    SECTION("Color interpolation") {
        Color lerped = red.lerp(blue, 0.5f);
        REQUIRE(lerped.r == Approx(0.5f));
        REQUIRE(lerped.g == Approx(0.0f));
        REQUIRE(lerped.b == Approx(0.5f));
    }
}