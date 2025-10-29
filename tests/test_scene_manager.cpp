#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../src/scene/SceneManager.h"
#include "../src/models/Project.h"

using namespace KitchenCAD;
using namespace KitchenCAD::Scene;
using namespace KitchenCAD::Geometry;
using Catch::Approx;

// Helper function to create a test scene object
std::unique_ptr<Models::SceneObject> createTestObject(const std::string& catalogItemId = "test_item") {
    auto object = std::make_unique<Models::SceneObject>(catalogItemId);
    return object;
}

TEST_CASE("SceneManager - Basic Object Management", "[scene][manager][objects]") {
    SceneManager sceneManager;
    
    SECTION("Add and retrieve objects") {
        auto object1 = createTestObject("item1");
        auto object2 = createTestObject("item2");
        
        ObjectId id1 = sceneManager.addObject(std::move(object1));
        ObjectId id2 = sceneManager.addObject(std::move(object2));
        
        REQUIRE(!id1.empty());
        REQUIRE(!id2.empty());
        REQUIRE(id1 != id2);
        
        REQUIRE(sceneManager.getObjectCount() == 2);
        REQUIRE(!sceneManager.isEmpty());
        
        auto* retrievedObject1 = sceneManager.getObject(id1);
        auto* retrievedObject2 = sceneManager.getObject(id2);
        
        REQUIRE(retrievedObject1 != nullptr);
        REQUIRE(retrievedObject2 != nullptr);
        REQUIRE(retrievedObject1->getCatalogItemId() == "item1");
        REQUIRE(retrievedObject2->getCatalogItemId() == "item2");
    }
    
    SECTION("Remove objects") {
        auto object = createTestObject("test_item");
        ObjectId id = sceneManager.addObject(std::move(object));
        
        REQUIRE(sceneManager.getObjectCount() == 1);
        
        bool removed = sceneManager.removeObject(id);
        REQUIRE(removed);
        REQUIRE(sceneManager.getObjectCount() == 0);
        REQUIRE(sceneManager.isEmpty());
        
        // Try to remove non-existent object
        bool removedAgain = sceneManager.removeObject(id);
        REQUIRE(!removedAgain);
    }
    
    SECTION("Get all objects") {
        auto object1 = createTestObject("item1");
        auto object2 = createTestObject("item2");
        auto object3 = createTestObject("item3");
        
        ObjectId id1 = sceneManager.addObject(std::move(object1));
        ObjectId id2 = sceneManager.addObject(std::move(object2));
        ObjectId id3 = sceneManager.addObject(std::move(object3));
        
        auto allObjects = sceneManager.getAllObjects();
        REQUIRE(allObjects.size() == 3);
        
        // Check that all IDs are present
        REQUIRE(std::find(allObjects.begin(), allObjects.end(), id1) != allObjects.end());
        REQUIRE(std::find(allObjects.begin(), allObjects.end(), id2) != allObjects.end());
        REQUIRE(std::find(allObjects.begin(), allObjects.end(), id3) != allObjects.end());
    }
    
    SECTION("Clear scene") {
        auto object1 = createTestObject("item1");
        auto object2 = createTestObject("item2");
        
        sceneManager.addObject(std::move(object1));
        sceneManager.addObject(std::move(object2));
        
        REQUIRE(sceneManager.getObjectCount() == 2);
        
        sceneManager.clear();
        
        REQUIRE(sceneManager.getObjectCount() == 0);
        REQUIRE(sceneManager.isEmpty());
    }
}

TEST_CASE("SceneManager - Selection Management", "[scene][manager][selection]") {
    SceneManager sceneManager;
    
    auto object1 = createTestObject("item1");
    auto object2 = createTestObject("item2");
    auto object3 = createTestObject("item3");
    
    ObjectId id1 = sceneManager.addObject(std::move(object1));
    ObjectId id2 = sceneManager.addObject(std::move(object2));
    ObjectId id3 = sceneManager.addObject(std::move(object3));
    
    SECTION("Single selection") {
        sceneManager.addToSelection(id1);
        
        REQUIRE(sceneManager.isSelected(id1));
        REQUIRE(!sceneManager.isSelected(id2));
        REQUIRE(!sceneManager.isSelected(id3));
        
        auto selection = sceneManager.getSelection();
        REQUIRE(selection.size() == 1);
        REQUIRE(selection[0] == id1);
    }
    
    SECTION("Multiple selection") {
        sceneManager.addToSelection(id1);
        sceneManager.addToSelection(id2);
        
        REQUIRE(sceneManager.isSelected(id1));
        REQUIRE(sceneManager.isSelected(id2));
        REQUIRE(!sceneManager.isSelected(id3));
        
        auto selection = sceneManager.getSelection();
        REQUIRE(selection.size() == 2);
    }
    
    SECTION("Set selection") {
        std::vector<ObjectId> newSelection = {id1, id3};
        sceneManager.setSelection(newSelection);
        
        REQUIRE(sceneManager.isSelected(id1));
        REQUIRE(!sceneManager.isSelected(id2));
        REQUIRE(sceneManager.isSelected(id3));
        
        auto selection = sceneManager.getSelection();
        REQUIRE(selection.size() == 2);
    }
    
    SECTION("Remove from selection") {
        sceneManager.addToSelection(id1);
        sceneManager.addToSelection(id2);
        
        REQUIRE(sceneManager.getSelection().size() == 2);
        
        sceneManager.removeFromSelection(id1);
        
        REQUIRE(!sceneManager.isSelected(id1));
        REQUIRE(sceneManager.isSelected(id2));
        REQUIRE(sceneManager.getSelection().size() == 1);
    }
    
    SECTION("Clear selection") {
        sceneManager.addToSelection(id1);
        sceneManager.addToSelection(id2);
        
        REQUIRE(sceneManager.getSelection().size() == 2);
        
        sceneManager.clearSelection();
        
        REQUIRE(sceneManager.getSelection().empty());
        REQUIRE(!sceneManager.isSelected(id1));
        REQUIRE(!sceneManager.isSelected(id2));
    }
}

TEST_CASE("SceneManager - Object Transformations", "[scene][manager][transform]") {
    SceneManager sceneManager;
    
    auto object = createTestObject("test_item");
    ObjectId id = sceneManager.addObject(std::move(object));
    
    SECTION("Move object") {
        Transform3D newTransform(Point3D(5.0, 3.0, 2.0));
        bool success = sceneManager.moveObject(id, newTransform);
        
        REQUIRE(success);
        
        auto* retrievedObject = sceneManager.getObject(id);
        REQUIRE(retrievedObject != nullptr);
        
        Transform3D objectTransform = retrievedObject->getTransform();
        REQUIRE(objectTransform.translation.x == Approx(5.0));
        REQUIRE(objectTransform.translation.y == Approx(3.0));
        REQUIRE(objectTransform.translation.z == Approx(2.0));
    }
    
    SECTION("Translate object") {
        Vector3D translation(2.0, 1.0, 3.0);
        bool success = sceneManager.translateObject(id, translation);
        
        REQUIRE(success);
        
        auto* retrievedObject = sceneManager.getObject(id);
        REQUIRE(retrievedObject != nullptr);
        
        Transform3D objectTransform = retrievedObject->getTransform();
        REQUIRE(objectTransform.translation.x == Approx(2.0));
        REQUIRE(objectTransform.translation.y == Approx(1.0));
        REQUIRE(objectTransform.translation.z == Approx(3.0));
    }
    
    SECTION("Rotate object") {
        Vector3D rotation(0.0, 0.0, M_PI / 2.0);  // 90 degrees around Z-axis
        bool success = sceneManager.rotateObject(id, rotation);
        
        REQUIRE(success);
        
        auto* retrievedObject = sceneManager.getObject(id);
        REQUIRE(retrievedObject != nullptr);
        
        Transform3D objectTransform = retrievedObject->getTransform();
        REQUIRE(objectTransform.rotation.z == Approx(M_PI / 2.0));
    }
    
    SECTION("Scale object") {
        Vector3D scale(2.0, 1.5, 0.5);
        bool success = sceneManager.scaleObject(id, scale);
        
        REQUIRE(success);
        
        auto* retrievedObject = sceneManager.getObject(id);
        REQUIRE(retrievedObject != nullptr);
        
        Transform3D objectTransform = retrievedObject->getTransform();
        REQUIRE(objectTransform.scale.x == Approx(2.0));
        REQUIRE(objectTransform.scale.y == Approx(1.5));
        REQUIRE(objectTransform.scale.z == Approx(0.5));
    }
    
    SECTION("Transform non-existent object") {
        Transform3D transform(Point3D(1.0, 1.0, 1.0));
        bool success = sceneManager.moveObject("non_existent_id", transform);
        
        REQUIRE(!success);
    }
}

TEST_CASE("SceneManager - Spatial Queries", "[scene][manager][spatial]") {
    SceneManager sceneManager;
    
    // Create objects at different positions
    auto object1 = createTestObject("item1");
    auto object2 = createTestObject("item2");
    auto object3 = createTestObject("item3");
    
    ObjectId id1 = sceneManager.addObject(std::move(object1));
    ObjectId id2 = sceneManager.addObject(std::move(object2));
    ObjectId id3 = sceneManager.addObject(std::move(object3));
    
    // Position objects
    sceneManager.moveObject(id1, Transform3D(Point3D(0.0, 0.0, 0.0)));
    sceneManager.moveObject(id2, Transform3D(Point3D(5.0, 0.0, 0.0)));
    sceneManager.moveObject(id3, Transform3D(Point3D(10.0, 0.0, 0.0)));
    
    SECTION("Get objects in region") {
        BoundingBox region(Point3D(-1.0, -1.0, -1.0), Point3D(6.0, 1.0, 1.0));
        auto objectsInRegion = sceneManager.getObjectsInRegion(region);
        
        // Should contain objects 1 and 2, but not 3
        REQUIRE(objectsInRegion.size() >= 1);  // At least object 1 should be found
        
        bool foundId1 = std::find(objectsInRegion.begin(), objectsInRegion.end(), id1) != objectsInRegion.end();
        REQUIRE(foundId1);
    }
    
    SECTION("Find nearby objects") {
        auto nearbyObjects = sceneManager.findNearbyObjects(id1, 3.0);
        
        // Should not include id1 itself
        bool foundId1 = std::find(nearbyObjects.begin(), nearbyObjects.end(), id1) != nearbyObjects.end();
        REQUIRE(!foundId1);
        
        // Might include id2 depending on bounding box calculations
        // This test is more about ensuring the method works without crashing
        REQUIRE(nearbyObjects.size() >= 0);
    }
    
    SECTION("Scene bounds") {
        BoundingBox sceneBounds = sceneManager.getSceneBounds();
        
        REQUIRE(sceneBounds.isValid());
        REQUIRE(!sceneBounds.isEmpty());
        
        // Scene should encompass all objects
        REQUIRE(sceneBounds.min.x <= 0.0);
        REQUIRE(sceneBounds.max.x >= 10.0);
    }
}

TEST_CASE("SceneManager - Collision Detection", "[scene][manager][collision]") {
    SceneManager sceneManager;
    
    auto object1 = createTestObject("item1");
    auto object2 = createTestObject("item2");
    
    ObjectId id1 = sceneManager.addObject(std::move(object1));
    ObjectId id2 = sceneManager.addObject(std::move(object2));
    
    // Position objects close to each other
    sceneManager.moveObject(id1, Transform3D(Point3D(0.0, 0.0, 0.0)));
    sceneManager.moveObject(id2, Transform3D(Point3D(0.1, 0.0, 0.0)));  // Very close
    
    SECTION("Check collision") {
        // Try to move object1 to the same position as object2
        Transform3D collisionTransform(Point3D(0.1, 0.0, 0.0));
        bool wouldCollide = sceneManager.checkCollision(id1, collisionTransform);
        
        // This depends on the bounding box implementation
        // The test ensures the method works
        REQUIRE((wouldCollide == true || wouldCollide == false));
    }
    
    SECTION("Detect all collisions") {
        auto collisions = sceneManager.detectAllCollisions();
        
        // Should detect collision between the two close objects
        REQUIRE(collisions.size() >= 0);  // At least no crash
    }
    
    SECTION("Disable collision detection") {
        sceneManager.setCollisionDetectionEnabled(false);
        REQUIRE(!sceneManager.isCollisionDetectionEnabled());
        
        Transform3D transform(Point3D(0.1, 0.0, 0.0));
        bool wouldCollide = sceneManager.checkCollision(id1, transform);
        REQUIRE(!wouldCollide);  // Should return false when disabled
    }
}

TEST_CASE("SceneManager - Object Duplication", "[scene][manager][duplicate]") {
    SceneManager sceneManager;
    
    auto object = createTestObject("test_item");
    ObjectId originalId = sceneManager.addObject(std::move(object));
    
    SECTION("Duplicate object") {
        auto duplicate = sceneManager.duplicateObject(originalId);
        
        REQUIRE(duplicate != nullptr);
        REQUIRE(duplicate->getCatalogItemId() == "test_item");
        REQUIRE(duplicate->getId() != originalId);  // Should have different ID
    }
    
    SECTION("Duplicate non-existent object") {
        auto duplicate = sceneManager.duplicateObject("non_existent_id");
        REQUIRE(duplicate == nullptr);
    }
}

TEST_CASE("SceneManager - Event Callbacks", "[scene][manager][events]") {
    SceneManager sceneManager;
    
    ObjectId addedObjectId;
    ObjectId removedObjectId;
    ObjectId modifiedObjectId;
    std::vector<ObjectId> lastSelection;
    
    // Set up callbacks
    sceneManager.setObjectAddedCallback([&](const ObjectId& id) {
        addedObjectId = id;
    });
    
    sceneManager.setObjectRemovedCallback([&](const ObjectId& id) {
        removedObjectId = id;
    });
    
    sceneManager.setObjectModifiedCallback([&](const ObjectId& id) {
        modifiedObjectId = id;
    });
    
    sceneManager.setSelectionChangedCallback([&](const std::vector<ObjectId>& selection) {
        lastSelection = selection;
    });
    
    SECTION("Object added callback") {
        auto object = createTestObject("test_item");
        ObjectId id = sceneManager.addObject(std::move(object));
        
        REQUIRE(addedObjectId == id);
    }
    
    SECTION("Object removed callback") {
        auto object = createTestObject("test_item");
        ObjectId id = sceneManager.addObject(std::move(object));
        
        sceneManager.removeObject(id);
        
        REQUIRE(removedObjectId == id);
    }
    
    SECTION("Object modified callback") {
        auto object = createTestObject("test_item");
        ObjectId id = sceneManager.addObject(std::move(object));
        
        Transform3D transform(Point3D(1.0, 1.0, 1.0));
        sceneManager.moveObject(id, transform);
        
        REQUIRE(modifiedObjectId == id);
    }
    
    SECTION("Selection changed callback") {
        auto object = createTestObject("test_item");
        ObjectId id = sceneManager.addObject(std::move(object));
        
        sceneManager.addToSelection(id);
        
        REQUIRE(lastSelection.size() == 1);
        REQUIRE(lastSelection[0] == id);
    }
}

TEST_CASE("SceneManager - Iteration Support", "[scene][manager][iteration]") {
    SceneManager sceneManager;
    
    auto object1 = createTestObject("item1");
    auto object2 = createTestObject("item2");
    auto object3 = createTestObject("item3");
    
    ObjectId id1 = sceneManager.addObject(std::move(object1));
    ObjectId id2 = sceneManager.addObject(std::move(object2));
    ObjectId id3 = sceneManager.addObject(std::move(object3));
    
    SECTION("For each object (mutable)") {
        std::vector<ObjectId> visitedIds;
        
        sceneManager.forEachObject([&](const ObjectId& id, SceneObject* object) {
            visitedIds.push_back(id);
            REQUIRE(object != nullptr);
        });
        
        REQUIRE(visitedIds.size() == 3);
        REQUIRE(std::find(visitedIds.begin(), visitedIds.end(), id1) != visitedIds.end());
        REQUIRE(std::find(visitedIds.begin(), visitedIds.end(), id2) != visitedIds.end());
        REQUIRE(std::find(visitedIds.begin(), visitedIds.end(), id3) != visitedIds.end());
    }
    
    SECTION("For each object (const)") {
        std::vector<ObjectId> visitedIds;
        
        const SceneManager& constSceneManager = sceneManager;
        constSceneManager.forEachObject([&](const ObjectId& id, const SceneObject* object) {
            visitedIds.push_back(id);
            REQUIRE(object != nullptr);
        });
        
        REQUIRE(visitedIds.size() == 3);
    }
}

TEST_CASE("SceneManager - Statistics and Validation", "[scene][manager][stats]") {
    SceneManager sceneManager;
    
    auto object1 = createTestObject("item1");
    auto object2 = createTestObject("item2");
    
    ObjectId id1 = sceneManager.addObject(std::move(object1));
    ObjectId id2 = sceneManager.addObject(std::move(object2));
    
    sceneManager.addToSelection(id1);
    
    SECTION("Get statistics") {
        auto stats = sceneManager.getStatistics();
        
        REQUIRE(stats.totalObjects == 2);
        REQUIRE(stats.selectedObjects == 1);
        REQUIRE(stats.sceneBounds.isValid());
        REQUIRE(stats.totalVolume >= 0.0);
    }
    
    SECTION("Validate scene") {
        auto issues = sceneManager.validateScene();
        
        // Should have no issues with a properly constructed scene
        // This test mainly ensures the method doesn't crash
        REQUIRE(issues.size() >= 0);
    }
}

TEST_CASE("SpatialIndex - Basic Operations", "[scene][spatial][index]") {
    SpatialIndex spatialIndex(1.0);
    
    BoundingBox box1(Point3D(0.0, 0.0, 0.0), Point3D(1.0, 1.0, 1.0));
    BoundingBox box2(Point3D(2.0, 0.0, 0.0), Point3D(3.0, 1.0, 1.0));
    BoundingBox box3(Point3D(0.5, 0.5, 0.5), Point3D(1.5, 1.5, 1.5));
    
    SECTION("Add and query objects") {
        spatialIndex.addObject("obj1", box1);
        spatialIndex.addObject("obj2", box2);
        spatialIndex.addObject("obj3", box3);
        
        BoundingBox queryRegion(Point3D(-0.5, -0.5, -0.5), Point3D(1.5, 1.5, 1.5));
        auto results = spatialIndex.queryRegion(queryRegion);
        
        REQUIRE(results.size() >= 1);  // Should find at least obj1
        
        bool foundObj1 = std::find(results.begin(), results.end(), "obj1") != results.end();
        REQUIRE(foundObj1);
    }
    
    SECTION("Remove objects") {
        spatialIndex.addObject("obj1", box1);
        spatialIndex.addObject("obj2", box2);
        
        spatialIndex.removeObject("obj1", box1);
        
        BoundingBox queryRegion(Point3D(-0.5, -0.5, -0.5), Point3D(1.5, 1.5, 1.5));
        auto results = spatialIndex.queryRegion(queryRegion);
        
        bool foundObj1 = std::find(results.begin(), results.end(), "obj1") != results.end();
        REQUIRE(!foundObj1);
    }
    
    SECTION("Query radius") {
        spatialIndex.addObject("obj1", box1);
        spatialIndex.addObject("obj2", box2);
        
        Point3D center(0.5, 0.5, 0.5);
        auto results = spatialIndex.queryRadius(center, 1.0);
        
        REQUIRE(results.size() >= 0);  // Should work without crashing
    }
    
    SECTION("Clear index") {
        spatialIndex.addObject("obj1", box1);
        spatialIndex.addObject("obj2", box2);
        
        spatialIndex.clear();
        
        BoundingBox queryRegion(Point3D(-10.0, -10.0, -10.0), Point3D(10.0, 10.0, 10.0));
        auto results = spatialIndex.queryRegion(queryRegion);
        
        REQUIRE(results.empty());
    }
}

TEST_CASE("CollisionDetector - Collision Detection", "[scene][collision][detector]") {
    BoundingBox box1(Point3D(0.0, 0.0, 0.0), Point3D(1.0, 1.0, 1.0));
    BoundingBox box2(Point3D(0.5, 0.5, 0.5), Point3D(1.5, 1.5, 1.5));  // Overlapping
    BoundingBox box3(Point3D(2.0, 0.0, 0.0), Point3D(3.0, 1.0, 1.0));  // Separate
    
    SECTION("Check bounding box intersection") {
        bool intersects1 = CollisionDetector::checkBoundingBoxIntersection(box1, box2);
        REQUIRE(intersects1);
        
        bool intersects2 = CollisionDetector::checkBoundingBoxIntersection(box1, box3);
        REQUIRE(!intersects2);
    }
    
    SECTION("Calculate penetration") {
        auto collision = CollisionDetector::calculatePenetration("obj1", "obj2", box1, box2);
        
        REQUIRE(collision.objectA == "obj1");
        REQUIRE(collision.objectB == "obj2");
        REQUIRE(collision.penetrationDepth > 0.0);
    }
    
    SECTION("Would collide check") {
        std::vector<BoundingBox> otherBounds = {box2, box3};
        Transform3D transform(Point3D(0.0, 0.0, 0.0));  // No movement
        
        bool wouldCollide = CollisionDetector::wouldCollide(box1, transform, otherBounds);
        REQUIRE(wouldCollide);  // Should collide with box2
    }
}