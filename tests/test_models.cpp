#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../src/models/Project.h"
#include "../src/models/CatalogItem.h"
#include <nlohmann/json.hpp>
#include <memory>

using namespace KitchenCAD::Models;
using namespace KitchenCAD;
namespace Geom = KitchenCAD::Geometry;
using Catch::Approx;
using json = nlohmann::json;

TEST_CASE("RoomDimensions operations", "[models][dimensions]") {
    SECTION("Construction and validation") {
        RoomDimensions dims(5.0, 3.0, 2.5);
        
        REQUIRE(dims.width == Approx(5.0));
        REQUIRE(dims.height == Approx(3.0));
        REQUIRE(dims.depth == Approx(2.5));
        REQUIRE(dims.isValid());
        REQUIRE(dims.volume() == Approx(37.5));
        
        // Invalid dimensions
        RoomDimensions invalidDims(0.0, -1.0, 2.0);
        REQUIRE_FALSE(invalidDims.isValid());
    }
    
    SECTION("Bounding box conversion") {
        RoomDimensions dims(4.0, 3.0, 2.0);
        auto bbox = dims.toBoundingBox();
        
        REQUIRE(bbox.min.x == Approx(0.0));
        REQUIRE(bbox.min.y == Approx(0.0));
        REQUIRE(bbox.min.z == Approx(0.0));
        REQUIRE(bbox.max.x == Approx(4.0));
        REQUIRE(bbox.max.y == Approx(3.0));
        REQUIRE(bbox.max.z == Approx(2.0));
    }
}

TEST_CASE("Wall operations", "[models][wall]") {
    SECTION("Wall creation and validation") {
        Geom::Point3D start(0.0, 0.0, 0.0);
        Geom::Point3D end(5.0, 0.0, 0.0);
        Wall wall("wall_1", start, end, 2.5, 0.1);
        
        REQUIRE(wall.id == "wall_1");
        REQUIRE(wall.length() == Approx(5.0));
        REQUIRE(wall.height == Approx(2.5));
        REQUIRE(wall.thickness == Approx(0.1));
        REQUIRE(wall.isValid());
        
        Geom::Vector3D direction = wall.direction();
        REQUIRE(direction.x == Approx(1.0));
        REQUIRE(direction.y == Approx(0.0));
        REQUIRE(direction.z == Approx(0.0));
    }
    
    SECTION("Invalid wall") {
        Geom::Point3D samePoint(1.0, 1.0, 1.0);
        Wall invalidWall("invalid", samePoint, samePoint, 2.5, 0.1);
        REQUIRE_FALSE(invalidWall.isValid());
    }
}

TEST_CASE("Opening operations", "[models][opening]") {
    SECTION("Door opening") {
        Opening door("door_1", "wall_1", "door", 0.5, 0.8, 2.0, 0.0);
        
        REQUIRE(door.id == "door_1");
        REQUIRE(door.wallId == "wall_1");
        REQUIRE(door.type == "door");
        REQUIRE(door.position == Approx(0.5));
        REQUIRE(door.width == Approx(0.8));
        REQUIRE(door.height == Approx(2.0));
        REQUIRE(door.sillHeight == Approx(0.0));
        REQUIRE(door.isValid());
    }
    
    SECTION("Window opening") {
        Opening window("window_1", "wall_1", "window", 0.3, 1.2, 1.0, 1.0);
        
        REQUIRE(window.type == "window");
        REQUIRE(window.sillHeight == Approx(1.0));
        REQUIRE(window.isValid());
    }
    
    SECTION("Invalid opening") {
        Opening invalid("invalid", "wall_1", "door", 1.5, 0.8, 2.0, 0.0); // position > 1.0
        REQUIRE_FALSE(invalid.isValid());
    }
}

TEST_CASE("Transform3D operations", "[models][transform]") {
    SECTION("Basic transform creation") {
        Models::Transform3D identity;
        REQUIRE(identity.isIdentity());
        
        Models::Transform3D transform(Geom::Point3D(5.0, 10.0, 15.0));
        REQUIRE(transform.translation.x == Approx(5.0));
        REQUIRE(transform.translation.y == Approx(10.0));
        REQUIRE(transform.translation.z == Approx(15.0));
    }
    
    SECTION("Transform with rotation and scale") {
        Models::Transform3D transform;
        transform.translation = Geom::Point3D(1.0, 2.0, 3.0);
        transform.rotation = Geom::Vector3D(0.1, 0.2, 0.3);
        transform.scale = Geom::Vector3D(2.0, 3.0, 4.0);
        
        REQUIRE(transform.translation.x == Approx(1.0));
        REQUIRE(transform.rotation.y == Approx(0.2));
        REQUIRE(transform.scale.z == Approx(4.0));
        REQUIRE_FALSE(transform.isIdentity());
    }
}

TEST_CASE("MaterialProperties operations", "[models][material]") {
    SECTION("Material creation") {
        MaterialProperties material("oak_wood", "Oak Wood");
        
        REQUIRE(material.id == "oak_wood");
        REQUIRE(material.name == "Oak Wood");
        REQUIRE(material.diffuseColor.r == Approx(1.0f));
        REQUIRE(material.diffuseColor.g == Approx(1.0f));
        REQUIRE(material.diffuseColor.b == Approx(1.0f));
        REQUIRE(material.roughness == Approx(0.5f));
        REQUIRE(material.metallic == Approx(0.0f));
        REQUIRE(material.pricePerSquareMeter == Approx(0.0));
    }
    
    SECTION("Material customization") {
        MaterialProperties material("custom_mat", "Custom Material");
        material.diffuseColor = {0.8f, 0.6f, 0.4f, 1.0f};
        material.roughness = 0.3f;
        material.metallic = 0.1f;
        material.pricePerSquareMeter = 45.50;
        material.supplier = "Test Supplier";
        material.code = "MAT001";
        
        REQUIRE(material.diffuseColor.r == Approx(0.8f));
        REQUIRE(material.diffuseColor.g == Approx(0.6f));
        REQUIRE(material.diffuseColor.b == Approx(0.4f));
        REQUIRE(material.roughness == Approx(0.3f));
        REQUIRE(material.metallic == Approx(0.1f));
        REQUIRE(material.pricePerSquareMeter == Approx(45.50));
        REQUIRE(material.supplier == "Test Supplier");
        REQUIRE(material.code == "MAT001");
    }
}

TEST_CASE("SceneObject operations", "[models][sceneobject]") {
    SECTION("SceneObject creation") {
        SceneObject object("catalog_item_123");
        
        REQUIRE(object.getCatalogItemId() == "catalog_item_123");
        REQUIRE_FALSE(object.getId().empty());
        REQUIRE(object.getTransform().isIdentity());
    }
    
    SECTION("SceneObject manipulation") {
        SceneObject object("test_item");
        
        // Set transform
        Models::Transform3D transform(Geom::Point3D(1.0, 2.0, 3.0));
        object.setTransform(transform);
        
        REQUIRE(object.getTransform().translation.x == Approx(1.0));
        REQUIRE(object.getTransform().translation.y == Approx(2.0));
        REQUIRE(object.getTransform().translation.z == Approx(3.0));
        
        // Set material
        MaterialProperties material("wood", "Wood Material");
        material.pricePerSquareMeter = 25.0;
        object.setMaterial(material);
        
        REQUIRE(object.getMaterial().id == "wood");
        REQUIRE(object.getMaterial().pricePerSquareMeter == Approx(25.0));
        
        // Set custom properties
        object.setCustomProperties("{\"custom_field\": \"custom_value\"}");
        REQUIRE(object.getCustomProperties() == "{\"custom_field\": \"custom_value\"}");
    }
    
    SECTION("SceneObject JSON serialization") {
        SceneObject object("test_catalog_item");
        object.setId("test_object_id");
        
        Models::Transform3D transform(Geom::Point3D(5.0, 10.0, 15.0));
        transform.rotation = Geom::Vector3D(0.1, 0.2, 0.3);
        transform.scale = Geom::Vector3D(1.5, 2.0, 2.5);
        object.setTransform(transform);
        
        MaterialProperties material("test_material", "Test Material");
        material.diffuseColor = {0.5f, 0.7f, 0.9f, 1.0f};
        material.roughness = 0.4f;
        material.pricePerSquareMeter = 30.0;
        object.setMaterial(material);
        
        object.setCustomProperties("{\"test\": true}");
        
        // Serialize to JSON
        json j = object.toJson();
        
        REQUIRE(j["id"] == "test_object_id");
        REQUIRE(j["catalogItemId"] == "test_catalog_item");
        REQUIRE(j["transform"]["translation"]["x"] == Approx(5.0));
        REQUIRE(j["transform"]["rotation"]["y"] == Approx(0.2));
        REQUIRE(j["transform"]["scale"]["z"] == Approx(2.5));
        REQUIRE(j["material"]["id"] == "test_material");
        REQUIRE(j["material"]["diffuseColor"]["g"] == Approx(0.7f));
        REQUIRE(j["customProperties"] == "{\"test\": true}");
        
        // Deserialize from JSON
        SceneObject deserializedObject;
        deserializedObject.fromJson(j);
        
        REQUIRE(deserializedObject.getId() == "test_object_id");
        REQUIRE(deserializedObject.getCatalogItemId() == "test_catalog_item");
        REQUIRE(deserializedObject.getTransform().translation.x == Approx(5.0));
        REQUIRE(deserializedObject.getTransform().rotation.y == Approx(0.2));
        REQUIRE(deserializedObject.getTransform().scale.z == Approx(2.5));
        REQUIRE(deserializedObject.getMaterial().id == "test_material");
        REQUIRE(deserializedObject.getMaterial().diffuseColor.g == Approx(0.7f));
        REQUIRE(deserializedObject.getCustomProperties() == "{\"test\": true}");
    }
}

TEST_CASE("Project creation and basic operations", "[models][project]") {
    SECTION("Project creation") {
        RoomDimensions dimensions(5.0, 3.0, 2.5);
        Project project("Test Kitchen", dimensions);
        
        REQUIRE(project.getName() == "Test Kitchen");
        REQUIRE(project.getDimensions().width == Approx(5.0));
        REQUIRE(project.getDimensions().height == Approx(3.0));
        REQUIRE(project.getDimensions().depth == Approx(2.5));
        REQUIRE(project.getObjectCount() == 0);
        REQUIRE_FALSE(project.getId().empty());
        REQUIRE(project.isValid());
    }
    
    SECTION("Project with ID") {
        RoomDimensions dimensions(4.0, 3.0, 2.0);
        Project project("custom_id", "Custom Project", dimensions);
        
        REQUIRE(project.getId() == "custom_id");
        REQUIRE(project.getName() == "Custom Project");
    }
    
    SECTION("Project validation") {
        // Valid project
        Project validProject("Valid Project", RoomDimensions(5.0, 3.0, 2.5));
        REQUIRE(validProject.isValid());
        
        // Invalid project - empty name
        Project invalidProject("", RoomDimensions(5.0, 3.0, 2.5));
        REQUIRE_FALSE(invalidProject.isValid());
        
        auto errors = invalidProject.validate();
        REQUIRE_FALSE(errors.empty());
        REQUIRE(std::find_if(errors.begin(), errors.end(), 
            [](const std::string& error) { return error.find("name") != std::string::npos; }) != errors.end());
        
        // Invalid project - invalid dimensions
        Project invalidDimsProject("Test", RoomDimensions(0.0, 0.0, 0.0));
        REQUIRE_FALSE(invalidDimsProject.isValid());
    }
}

TEST_CASE("Project object management", "[models][project]") {
    SECTION("Adding and removing objects") {
        Project project("Test Kitchen", RoomDimensions(5.0, 3.0, 2.5));
        
        // Add objects
        auto object1 = std::make_unique<SceneObject>("catalog_item_1");
        auto object2 = std::make_unique<SceneObject>("catalog_item_2");
        
        std::string obj1Id = project.addObject(std::move(object1));
        std::string obj2Id = project.addObject(std::move(object2));
        
        REQUIRE_FALSE(obj1Id.empty());
        REQUIRE_FALSE(obj2Id.empty());
        REQUIRE(obj1Id != obj2Id);
        REQUIRE(project.getObjectCount() == 2);
        
        // Get objects
        SceneObject* retrievedObj1 = project.getObject(obj1Id);
        REQUIRE(retrievedObj1 != nullptr);
        REQUIRE(retrievedObj1->getId() == obj1Id);
        REQUIRE(retrievedObj1->getCatalogItemId() == "catalog_item_1");
        
        // Remove object
        REQUIRE(project.removeObject(obj1Id));
        REQUIRE(project.getObjectCount() == 1);
        REQUIRE(project.getObject(obj1Id) == nullptr);
        
        // Try to remove non-existent object
        REQUIRE_FALSE(project.removeObject("non_existent_id"));
    }
    
    SECTION("Adding null object") {
        Project project("Test", RoomDimensions(1.0, 1.0, 1.0));
        
        std::string result = project.addObject(nullptr);
        REQUIRE(result.empty());
        REQUIRE(project.getObjectCount() == 0);
    }
}

TEST_CASE("Project wall management", "[models][project]") {
    SECTION("Adding and managing walls") {
        Project project("Test Kitchen", RoomDimensions(5.0, 3.0, 2.5));
        
        // Add walls
        Wall wall1("wall_1", Geom::Point3D(0, 0, 0), Geom::Point3D(5, 0, 0), 2.5, 0.1);
        Wall wall2("wall_2", Geom::Point3D(5, 0, 0), Geom::Point3D(5, 3, 0), 2.5, 0.1);
        
        project.addWall(wall1);
        project.addWall(wall2);
        
        REQUIRE(project.getWalls().size() == 2);
        
        // Get wall
        const Wall* retrievedWall = project.getWall("wall_1");
        REQUIRE(retrievedWall != nullptr);
        REQUIRE(retrievedWall->id == "wall_1");
        REQUIRE(retrievedWall->length() == Approx(5.0));
        
        // Remove wall
        REQUIRE(project.removeWall("wall_1"));
        REQUIRE(project.getWalls().size() == 1);
        REQUIRE(project.getWall("wall_1") == nullptr);
        
        // Try to remove non-existent wall
        REQUIRE_FALSE(project.removeWall("non_existent_wall"));
    }
}

TEST_CASE("Project opening management", "[models][project]") {
    SECTION("Adding and managing openings") {
        Project project("Test Kitchen", RoomDimensions(5.0, 3.0, 2.5));
        
        // Add wall first
        Wall wall("wall_1", Geom::Point3D(0, 0, 0), Geom::Point3D(5, 0, 0), 2.5, 0.1);
        project.addWall(wall);
        
        // Add openings
        Opening door("door_1", "wall_1", "door", 0.2, 0.8, 2.0, 0.0);
        Opening window("window_1", "wall_1", "window", 0.7, 1.2, 1.0, 1.0);
        
        project.addOpening(door);
        project.addOpening(window);
        
        REQUIRE(project.getOpenings().size() == 2);
        
        // Get opening
        const Opening* retrievedOpening = project.getOpening("door_1");
        REQUIRE(retrievedOpening != nullptr);
        REQUIRE(retrievedOpening->id == "door_1");
        REQUIRE(retrievedOpening->type == "door");
        
        // Remove opening
        REQUIRE(project.removeOpening("door_1"));
        REQUIRE(project.getOpenings().size() == 1);
        REQUIRE(project.getOpening("door_1") == nullptr);
    }
    
    SECTION("Opening validation with walls") {
        Project project("Test Kitchen", RoomDimensions(5.0, 3.0, 2.5));
        
        // Add opening without corresponding wall
        Opening orphanOpening("orphan", "non_existent_wall", "door", 0.5, 0.8, 2.0, 0.0);
        project.addOpening(orphanOpening);
        
        // Project should be invalid due to orphan opening
        REQUIRE_FALSE(project.isValid());
        
        auto errors = project.validate();
        REQUIRE_FALSE(errors.empty());
        REQUIRE(std::find_if(errors.begin(), errors.end(),
            [](const std::string& error) { return error.find("non-existent wall") != std::string::npos; }) != errors.end());
    }
}

TEST_CASE("Project calculations", "[models][project]") {
    SECTION("Price calculation") {
        Project project("Test Kitchen", RoomDimensions(5.0, 3.0, 2.5));
        
        // Add objects with materials
        auto object1 = std::make_unique<SceneObject>("item1");
        MaterialProperties material1("mat1", "Material 1");
        material1.pricePerSquareMeter = 25.0;
        object1->setMaterial(material1);
        
        auto object2 = std::make_unique<SceneObject>("item2");
        MaterialProperties material2("mat2", "Material 2");
        material2.pricePerSquareMeter = 35.0;
        object2->setMaterial(material2);
        
        project.addObject(std::move(object1));
        project.addObject(std::move(object2));
        
        double totalPrice = project.calculateTotalPrice();
        REQUIRE(totalPrice == Approx(60.0)); // 25.0 + 35.0
    }
    
    SECTION("Bounding box calculation") {
        Project project("Test Kitchen", RoomDimensions(5.0, 3.0, 2.5));
        
        // For empty project, should return room dimensions
        auto bbox = project.calculateBoundingBox();
        REQUIRE(bbox.size().x == Approx(5.0));
        REQUIRE(bbox.size().y == Approx(3.0));
        REQUIRE(bbox.size().z == Approx(2.5));
    }
}

TEST_CASE("Project JSON serialization", "[models][project]") {
    SECTION("Complete project serialization") {
        Project project("Serialization Test", RoomDimensions(6.0, 4.0, 3.0));
        project.setDescription("Test project for serialization");
        
        // Add objects
        auto object = std::make_unique<SceneObject>("test_catalog_item");
        Models::Transform3D transform(Geom::Point3D(1.0, 2.0, 3.0));
        object->setTransform(transform);
        
        MaterialProperties material("test_material", "Test Material");
        material.pricePerSquareMeter = 50.0;
        object->setMaterial(material);
        
        project.addObject(std::move(object));
        
        // Add walls
        Wall wall("wall_1", Geom::Point3D(0, 0, 0), Geom::Point3D(6, 0, 0), 3.0, 0.1);
        project.addWall(wall);
        
        // Add openings
        Opening door("door_1", "wall_1", "door", 0.5, 0.8, 2.0, 0.0);
        project.addOpening(door);
        
        // Serialize to JSON
        json j = project.toJson();
        
        REQUIRE(j["name"] == "Serialization Test");
        REQUIRE(j["description"] == "Test project for serialization");
        REQUIRE(j["dimensions"]["width"] == Approx(6.0));
        REQUIRE(j["dimensions"]["height"] == Approx(4.0));
        REQUIRE(j["dimensions"]["depth"] == Approx(3.0));
        REQUIRE(j["objects"].size() == 1);
        REQUIRE(j["walls"].size() == 1);
        REQUIRE(j["openings"].size() == 1);
        
        // Deserialize from JSON
        Project deserializedProject;
        deserializedProject.fromJson(j);
        
        REQUIRE(deserializedProject.getName() == "Serialization Test");
        REQUIRE(deserializedProject.getDescription() == "Test project for serialization");
        REQUIRE(deserializedProject.getDimensions().width == Approx(6.0));
        REQUIRE(deserializedProject.getObjectCount() == 1);
        REQUIRE(deserializedProject.getWalls().size() == 1);
        REQUIRE(deserializedProject.getOpenings().size() == 1);
        
        // Verify object details
        const auto& objects = deserializedProject.getObjects();
        REQUIRE(objects[0]->getCatalogItemId() == "test_catalog_item");
        REQUIRE(objects[0]->getTransform().translation.x == Approx(1.0));
        REQUIRE(objects[0]->getMaterial().pricePerSquareMeter == Approx(50.0));
        
        // Verify wall details
        const Wall* deserializedWall = deserializedProject.getWall("wall_1");
        REQUIRE(deserializedWall != nullptr);
        REQUIRE(deserializedWall->length() == Approx(6.0));
        
        // Verify opening details
        const Opening* deserializedOpening = deserializedProject.getOpening("door_1");
        REQUIRE(deserializedOpening != nullptr);
        REQUIRE(deserializedOpening->type == "door");
    }
    
    SECTION("Scene serialization") {
        Project project("Scene Test", RoomDimensions(3.0, 3.0, 3.0));
        
        // Add multiple objects
        for (int i = 0; i < 3; ++i) {
            auto object = std::make_unique<SceneObject>("item_" + std::to_string(i));
            Models::Transform3D transform(Geom::Point3D(i * 1.0, 0.0, 0.0));
            object->setTransform(transform);
            project.addObject(std::move(object));
        }
        
        // Serialize scene
        json sceneJson = project.serializeSceneToJson();
        REQUIRE(sceneJson["objects"].size() == 3);
        
        // Clear and load scene
        Project newProject("Empty", RoomDimensions(3.0, 3.0, 3.0));
        newProject.loadSceneFromJson(sceneJson);
        
        REQUIRE(newProject.getObjectCount() == 3);
        
        // Verify object positions
        const auto& objects = newProject.getObjects();
        for (size_t i = 0; i < objects.size(); ++i) {
            REQUIRE(objects[i]->getTransform().translation.x == Approx(i * 1.0));
        }
    }
}

TEST_CASE("Project metadata operations", "[models][project]") {
    SECTION("ProjectInfo conversion") {
        Project project("Info Test", RoomDimensions(5.0, 3.0, 2.5));
        project.setDescription("Test description");
        
        // Add some objects to test object count
        auto object1 = std::make_unique<SceneObject>("item1");
        auto object2 = std::make_unique<SceneObject>("item2");
        project.addObject(std::move(object1));
        project.addObject(std::move(object2));
        
        ProjectInfo info = project.toProjectInfo();
        
        REQUIRE(info.id == project.getId());
        REQUIRE(info.name == "Info Test");
        REQUIRE(info.description == "Test description");
        REQUIRE(info.roomWidth == Approx(5.0));
        REQUIRE(info.roomHeight == Approx(3.0));
        REQUIRE(info.roomDepth == Approx(2.5));
        REQUIRE(info.objectCount == 2);
        REQUIRE_FALSE(info.createdAt.empty());
        REQUIRE_FALSE(info.updatedAt.empty());
    }
    
    SECTION("ProjectMetadata conversion") {
        Project project("Metadata Test", RoomDimensions(4.0, 2.0, 1.5));
        project.setDescription("Metadata description");
        
        ProjectMetadata metadata = project.toProjectMetadata();
        
        REQUIRE(metadata.name == "Metadata Test");
        REQUIRE(metadata.description == "Metadata description");
        REQUIRE(metadata.roomWidth == Approx(4.0));
        REQUIRE(metadata.roomHeight == Approx(2.0));
        REQUIRE(metadata.roomDepth == Approx(1.5));
        
        // Update project from metadata
        ProjectMetadata newMetadata;
        newMetadata.name = "Updated Name";
        newMetadata.description = "Updated Description";
        newMetadata.roomWidth = 10.0;
        newMetadata.roomHeight = 8.0;
        newMetadata.roomDepth = 6.0;
        
        project.updateFromMetadata(newMetadata);
        
        REQUIRE(project.getName() == "Updated Name");
        REQUIRE(project.getDescription() == "Updated Description");
        REQUIRE(project.getDimensions().width == Approx(10.0));
        REQUIRE(project.getDimensions().height == Approx(8.0));
        REQUIRE(project.getDimensions().depth == Approx(6.0));
    }
}

TEST_CASE("Dimensions3D operations", "[models][catalog][dimensions]") {
    SECTION("Construction and validation") {
        Dimensions3D dims(0.6, 0.85, 0.6);
        
        REQUIRE(dims.width == Approx(0.6));
        REQUIRE(dims.height == Approx(0.85));
        REQUIRE(dims.depth == Approx(0.6));
        REQUIRE(dims.isValid());
        REQUIRE(dims.volume() == Approx(0.306));
        
        // Invalid dimensions
        Dimensions3D invalidDims(-1.0, 0.0, 2.0);
        REQUIRE_FALSE(invalidDims.isValid());
    }
    
    SECTION("Conversions") {
        Dimensions3D dims(2.0, 3.0, 4.0);
        
        Vector3D vec = dims.toVector();
        REQUIRE(vec.x == Approx(2.0));
        REQUIRE(vec.y == Approx(3.0));
        REQUIRE(vec.z == Approx(4.0));
        
        auto bbox = dims.toBoundingBox();
        REQUIRE(bbox.size().x == Approx(2.0));
        REQUIRE(bbox.size().y == Approx(3.0));
        REQUIRE(bbox.size().z == Approx(4.0));
        
        auto bboxWithOrigin = dims.toBoundingBox(Geom::Point3D(1.0, 1.0, 1.0));
        REQUIRE(bboxWithOrigin.min.x == Approx(1.0));
        REQUIRE(bboxWithOrigin.max.x == Approx(3.0));
    }
}

TEST_CASE("MaterialOption operations", "[models][catalog][material]") {
    SECTION("MaterialOption creation") {
        MaterialOption option("oak_wood", "Oak Wood", 25.50);
        
        REQUIRE(option.id == "oak_wood");
        REQUIRE(option.name == "Oak Wood");
        REQUIRE(option.priceModifier == Approx(25.50));
        REQUIRE(option.texturePath.empty());
        REQUIRE(option.properties.empty());
    }
    
    SECTION("MaterialOption JSON serialization") {
        MaterialOption option("maple_wood", "Maple Wood", 35.75);
        option.texturePath = "/textures/maple.jpg";
        option.properties = "{\"grain\": \"fine\", \"hardness\": \"medium\"}";
        
        json j = option.toJson();
        
        REQUIRE(j["id"] == "maple_wood");
        REQUIRE(j["name"] == "Maple Wood");
        REQUIRE(j["priceModifier"] == Approx(35.75));
        REQUIRE(j["texturePath"] == "/textures/maple.jpg");
        REQUIRE(j["properties"] == "{\"grain\": \"fine\", \"hardness\": \"medium\"}");
        
        MaterialOption deserializedOption;
        deserializedOption.fromJson(j);
        
        REQUIRE(deserializedOption.id == "maple_wood");
        REQUIRE(deserializedOption.name == "Maple Wood");
        REQUIRE(deserializedOption.priceModifier == Approx(35.75));
        REQUIRE(deserializedOption.texturePath == "/textures/maple.jpg");
        REQUIRE(deserializedOption.properties == "{\"grain\": \"fine\", \"hardness\": \"medium\"}");
    }
}

TEST_CASE("Specifications operations", "[models][catalog][specs]") {
    SECTION("Specifications creation and serialization") {
        Specifications specs;
        specs.material = "Plywood";
        specs.finish = "Laminate";
        specs.hardware = "Soft-close hinges";
        specs.weight = 15.5;
        specs.loadCapacity = 25.0;
        specs.installationType = "Wall-mounted";
        specs.features = {"Adjustable shelves", "Soft-close doors", "LED lighting"};
        specs.additionalInfo = "{\"warranty\": \"5 years\"}";
        
        json j = specs.toJson();
        
        REQUIRE(j["material"] == "Plywood");
        REQUIRE(j["finish"] == "Laminate");
        REQUIRE(j["hardware"] == "Soft-close hinges");
        REQUIRE(j["weight"] == Approx(15.5));
        REQUIRE(j["loadCapacity"] == Approx(25.0));
        REQUIRE(j["installationType"] == "Wall-mounted");
        REQUIRE(j["features"].size() == 3);
        REQUIRE(j["additionalInfo"] == "{\"warranty\": \"5 years\"}");
        
        Specifications deserializedSpecs;
        deserializedSpecs.fromJson(j);
        
        REQUIRE(deserializedSpecs.material == "Plywood");
        REQUIRE(deserializedSpecs.weight == Approx(15.5));
        REQUIRE(deserializedSpecs.features.size() == 3);
        REQUIRE(deserializedSpecs.features[0] == "Adjustable shelves");
    }
}

TEST_CASE("CatalogItem creation and validation", "[models][catalog]") {
    SECTION("Basic catalog item creation") {
        CatalogItem item("base_60", "Base Cabinet 60cm", "base_cabinets");
        
        REQUIRE(item.getId() == "base_60");
        REQUIRE(item.getName() == "Base Cabinet 60cm");
        REQUIRE(item.getCategory() == "base_cabinets");
        REQUIRE(item.getBasePrice() == Approx(0.0));
        REQUIRE(item.getMaterialOptions().empty());
        
        // Item should be invalid without dimensions
        REQUIRE_FALSE(item.isValid());
        
        // Set dimensions and price
        item.setDimensions(Dimensions3D(0.6, 0.85, 0.6));
        item.setBasePrice(299.99);
        
        REQUIRE(item.isValid());
        REQUIRE(item.getDimensions().width == Approx(0.6));
        REQUIRE(item.getBasePrice() == Approx(299.99));
    }
    
    SECTION("CatalogItem validation") {
        CatalogItem validItem("valid_item", "Valid Item", "test_category");
        validItem.setDimensions(Dimensions3D(1.0, 1.0, 1.0));
        validItem.setBasePrice(100.0);
        
        REQUIRE(validItem.isValid());
        REQUIRE(validItem.validate().empty());
        
        // Test invalid items
        CatalogItem invalidItem("", "", "");
        auto errors = invalidItem.validate();
        REQUIRE_FALSE(errors.empty());
        REQUIRE(errors.size() >= 3); // ID, name, category errors
        
        // Test negative price
        CatalogItem negativePrice("item", "Item", "category");
        negativePrice.setDimensions(Dimensions3D(1.0, 1.0, 1.0));
        negativePrice.setBasePrice(-10.0);
        
        errors = negativePrice.validate();
        REQUIRE_FALSE(errors.empty());
        REQUIRE(std::find_if(errors.begin(), errors.end(),
            [](const std::string& error) { return error.find("negative") != std::string::npos; }) != errors.end());
    }
}

TEST_CASE("CatalogItem material options", "[models][catalog]") {
    SECTION("Adding and managing material options") {
        CatalogItem item("test_item", "Test Item", "test_category");
        item.setDimensions(Dimensions3D(1.0, 1.0, 1.0));
        item.setBasePrice(100.0);
        
        // Add material options
        MaterialOption option1("wood_oak", "Oak Wood", 25.0);
        MaterialOption option2("wood_maple", "Maple Wood", 35.0);
        MaterialOption option3("laminate_white", "White Laminate", -10.0);
        
        item.addMaterialOption(option1);
        item.addMaterialOption(option2);
        item.addMaterialOption(option3);
        
        REQUIRE(item.getMaterialOptions().size() == 3);
        
        // Get specific material option
        const MaterialOption* oakOption = item.getMaterialOption("wood_oak");
        REQUIRE(oakOption != nullptr);
        REQUIRE(oakOption->name == "Oak Wood");
        REQUIRE(oakOption->priceModifier == Approx(25.0));
        
        // Test price calculation with materials
        REQUIRE(item.getPrice() == Approx(100.0)); // Base price
        REQUIRE(item.getPrice("wood_oak") == Approx(125.0)); // Base + oak modifier
        REQUIRE(item.getPrice("wood_maple") == Approx(135.0)); // Base + maple modifier
        REQUIRE(item.getPrice("laminate_white") == Approx(90.0)); // Base + laminate modifier
        REQUIRE(item.getPrice("non_existent") == Approx(100.0)); // Non-existent material returns base price
        
        // Test getPriceWithMaterial
        REQUIRE(item.getPriceWithMaterial(option1) == Approx(125.0));
        
        // Remove material option
        REQUIRE(item.removeMaterialOption("wood_oak"));
        REQUIRE(item.getMaterialOptions().size() == 2);
        REQUIRE(item.getMaterialOption("wood_oak") == nullptr);
        
        // Try to remove non-existent option
        REQUIRE_FALSE(item.removeMaterialOption("non_existent"));
        
        // Clear all options
        item.clearMaterialOptions();
        REQUIRE(item.getMaterialOptions().empty());
    }
    
    SECTION("Duplicate material option handling") {
        CatalogItem item("test_item", "Test Item", "test_category");
        
        MaterialOption option1("wood_oak", "Oak Wood", 25.0);
        MaterialOption option2("wood_oak", "Oak Wood Updated", 30.0); // Same ID
        
        item.addMaterialOption(option1);
        REQUIRE(item.getMaterialOptions().size() == 1);
        
        // Adding option with same ID should replace the existing one
        item.addMaterialOption(option2);
        REQUIRE(item.getMaterialOptions().size() == 1);
        
        const MaterialOption* oakOption = item.getMaterialOption("wood_oak");
        REQUIRE(oakOption != nullptr);
        REQUIRE(oakOption->name == "Oak Wood Updated");
        REQUIRE(oakOption->priceModifier == Approx(30.0));
    }
}

TEST_CASE("CatalogItem search and filtering", "[models][catalog]") {
    SECTION("Search functionality") {
        CatalogItem item("base_cabinet_60", "Base Cabinet 60cm Oak", "base_cabinets");
        item.setDimensions(Dimensions3D(0.6, 0.85, 0.6));
        item.setBasePrice(299.99);
        
        Specifications specs;
        specs.material = "Oak plywood";
        specs.features = {"Soft-close doors", "Adjustable shelves"};
        item.setSpecifications(specs);
        
        // Test search matches
        REQUIRE(item.matchesSearch(""));           // Empty search matches all
        REQUIRE(item.matchesSearch("Cabinet"));    // Name match
        REQUIRE(item.matchesSearch("cabinet"));    // Case insensitive
        REQUIRE(item.matchesSearch("Base"));       // Partial name match
        REQUIRE(item.matchesSearch("Oak"));        // Name and material match
        REQUIRE(item.matchesSearch("base_cabinets")); // Category match
        REQUIRE(item.matchesSearch("Soft-close")); // Feature match
        
        // Test search non-matches
        REQUIRE_FALSE(item.matchesSearch("Wall"));
        REQUIRE_FALSE(item.matchesSearch("Stainless"));
        REQUIRE_FALSE(item.matchesSearch("xyz123"));
    }
    
    SECTION("Category filtering") {
        CatalogItem item("test_item", "Test Item", "base_cabinets");
        
        REQUIRE(item.matchesCategory(""));              // Empty category matches all
        REQUIRE(item.matchesCategory("base_cabinets")); // Exact match
        REQUIRE_FALSE(item.matchesCategory("wall_cabinets")); // Different category
    }
    
    SECTION("Dimension filtering") {
        CatalogItem item("test_item", "Test Item", "test_category");
        item.setDimensions(Dimensions3D(0.6, 0.85, 0.6));
        
        // Test dimension ranges
        Dimensions3D minDims(0.5, 0.8, 0.5);
        Dimensions3D maxDims(0.7, 0.9, 0.7);
        REQUIRE(item.matchesDimensions(minDims, maxDims));
        
        // Test outside range
        Dimensions3D tooSmallMin(0.7, 0.9, 0.7);
        Dimensions3D tooSmallMax(0.8, 1.0, 0.8);
        REQUIRE_FALSE(item.matchesDimensions(tooSmallMin, tooSmallMax));
        
        Dimensions3D tooLargeMin(0.1, 0.1, 0.1);
        Dimensions3D tooLargeMax(0.5, 0.7, 0.5);
        REQUIRE_FALSE(item.matchesDimensions(tooLargeMin, tooLargeMax));
    }
    
    SECTION("Price range filtering") {
        CatalogItem item("test_item", "Test Item", "test_category");
        item.setBasePrice(150.0);
        
        REQUIRE(item.matchesPriceRange(100.0, 200.0));  // Within range
        REQUIRE(item.matchesPriceRange(150.0, 150.0));  // Exact match
        REQUIRE_FALSE(item.matchesPriceRange(200.0, 300.0)); // Above range
        REQUIRE_FALSE(item.matchesPriceRange(50.0, 100.0));  // Below range
    }
}

TEST_CASE("CatalogItem JSON serialization", "[models][catalog]") {
    SECTION("Complete serialization") {
        CatalogItem item("serialization_test", "Serialization Test Item", "test_category");
        item.setDimensions(Dimensions3D(1.2, 0.8, 0.4));
        item.setBasePrice(199.99);
        item.setModelPath("/models/test_item.obj");
        item.setThumbnailPath("/thumbnails/test_item.jpg");
        
        // Set specifications
        Specifications specs;
        specs.material = "MDF";
        specs.finish = "Melamine";
        specs.weight = 12.5;
        specs.loadCapacity = 20.0;
        specs.features = {"Feature 1", "Feature 2"};
        item.setSpecifications(specs);
        
        // Add material options
        MaterialOption option1("mat1", "Material 1", 10.0);
        option1.texturePath = "/textures/mat1.jpg";
        MaterialOption option2("mat2", "Material 2", 20.0);
        item.addMaterialOption(option1);
        item.addMaterialOption(option2);
        
        // Serialize to JSON
        json j = item.toJson();
        
        REQUIRE(j["id"] == "serialization_test");
        REQUIRE(j["name"] == "Serialization Test Item");
        REQUIRE(j["category"] == "test_category");
        REQUIRE(j["basePrice"] == Approx(199.99));
        REQUIRE(j["modelPath"] == "/models/test_item.obj");
        REQUIRE(j["thumbnailPath"] == "/thumbnails/test_item.jpg");
        
        REQUIRE(j["dimensions"]["width"] == Approx(1.2));
        REQUIRE(j["dimensions"]["height"] == Approx(0.8));
        REQUIRE(j["dimensions"]["depth"] == Approx(0.4));
        
        REQUIRE(j["specifications"]["material"] == "MDF");
        REQUIRE(j["specifications"]["weight"] == Approx(12.5));
        REQUIRE(j["specifications"]["features"].size() == 2);
        
        REQUIRE(j["materialOptions"].size() == 2);
        REQUIRE(j["materialOptions"][0]["id"] == "mat1");
        REQUIRE(j["materialOptions"][0]["priceModifier"] == Approx(10.0));
        
        REQUIRE_FALSE(j["createdAt"].empty());
        REQUIRE_FALSE(j["updatedAt"].empty());
        
        // Deserialize from JSON
        CatalogItem deserializedItem;
        deserializedItem.fromJson(j);
        
        REQUIRE(deserializedItem.getId() == "serialization_test");
        REQUIRE(deserializedItem.getName() == "Serialization Test Item");
        REQUIRE(deserializedItem.getCategory() == "test_category");
        REQUIRE(deserializedItem.getBasePrice() == Approx(199.99));
        REQUIRE(deserializedItem.getModelPath() == "/models/test_item.obj");
        REQUIRE(deserializedItem.getThumbnailPath() == "/thumbnails/test_item.jpg");
        
        REQUIRE(deserializedItem.getDimensions().width == Approx(1.2));
        REQUIRE(deserializedItem.getDimensions().height == Approx(0.8));
        REQUIRE(deserializedItem.getDimensions().depth == Approx(0.4));
        
        REQUIRE(deserializedItem.getSpecifications().material == "MDF");
        REQUIRE(deserializedItem.getSpecifications().weight == Approx(12.5));
        REQUIRE(deserializedItem.getSpecifications().features.size() == 2);
        
        REQUIRE(deserializedItem.getMaterialOptions().size() == 2);
        const MaterialOption* mat1 = deserializedItem.getMaterialOption("mat1");
        REQUIRE(mat1 != nullptr);
        REQUIRE(mat1->priceModifier == Approx(10.0));
        REQUIRE(mat1->texturePath == "/textures/mat1.jpg");
    }
}

TEST_CASE("CatalogFilter operations", "[models][catalog][filter]") {
    SECTION("Filter creation and matching") {
        // Create test items
        CatalogItem item1("base_60", "Base Cabinet 60cm", "base_cabinets");
        item1.setDimensions(Dimensions3D(0.6, 0.85, 0.6));
        item1.setBasePrice(299.99);
        
        Specifications specs1;
        specs1.features = {"Soft-close doors", "Adjustable shelves"};
        item1.setSpecifications(specs1);
        
        CatalogItem item2("wall_80", "Wall Cabinet 80cm", "wall_cabinets");
        item2.setDimensions(Dimensions3D(0.8, 0.7, 0.35));
        item2.setBasePrice(199.99);
        
        Specifications specs2;
        specs2.features = {"Glass doors", "LED lighting"};
        item2.setSpecifications(specs2);
        
        // Test empty filter (matches all)
        CatalogFilter emptyFilter;
        REQUIRE(emptyFilter.matches(item1));
        REQUIRE(emptyFilter.matches(item2));
        
        // Test search term filter
        CatalogFilter searchFilter;
        searchFilter.searchTerm = "Base";
        REQUIRE(searchFilter.matches(item1));
        REQUIRE_FALSE(searchFilter.matches(item2));
        
        // Test category filter
        CatalogFilter categoryFilter;
        categoryFilter.categories = {"base_cabinets"};
        REQUIRE(categoryFilter.matches(item1));
        REQUIRE_FALSE(categoryFilter.matches(item2));
        
        // Test multiple categories
        categoryFilter.categories = {"base_cabinets", "wall_cabinets"};
        REQUIRE(categoryFilter.matches(item1));
        REQUIRE(categoryFilter.matches(item2));
        
        // Test dimension filter
        CatalogFilter dimensionFilter;
        dimensionFilter.minDimensions = Dimensions3D(0.5, 0.8, 0.5);
        dimensionFilter.maxDimensions = Dimensions3D(0.7, 0.9, 0.7);
        REQUIRE(dimensionFilter.matches(item1));
        REQUIRE_FALSE(dimensionFilter.matches(item2)); // Height too low
        
        // Test price filter
        CatalogFilter priceFilter;
        priceFilter.minPrice = 250.0;
        priceFilter.maxPrice = 350.0;
        REQUIRE(priceFilter.matches(item1));
        REQUIRE_FALSE(priceFilter.matches(item2)); // Price too low
        
        // Test feature filter
        CatalogFilter featureFilter;
        featureFilter.features = {"Soft-close doors"};
        REQUIRE(featureFilter.matches(item1));
        REQUIRE_FALSE(featureFilter.matches(item2));
        
        // Test multiple features (all must be present)
        featureFilter.features = {"Soft-close doors", "Adjustable shelves"};
        REQUIRE(featureFilter.matches(item1));
        REQUIRE_FALSE(featureFilter.matches(item2));
        
        featureFilter.features = {"Soft-close doors", "Non-existent feature"};
        REQUIRE_FALSE(featureFilter.matches(item1)); // Missing one feature
    }
    
    SECTION("Filter JSON serialization") {
        CatalogFilter filter;
        filter.searchTerm = "Cabinet";
        filter.categories = {"base_cabinets", "wall_cabinets"};
        filter.minDimensions = Dimensions3D(0.5, 0.7, 0.3);
        filter.maxDimensions = Dimensions3D(1.0, 1.0, 0.8);
        filter.minPrice = 100.0;
        filter.maxPrice = 500.0;
        filter.features = {"Soft-close doors", "LED lighting"};
        
        json j = filter.toJson();
        
        REQUIRE(j["searchTerm"] == "Cabinet");
        REQUIRE(j["categories"].size() == 2);
        REQUIRE(j["minDimensions"]["width"] == Approx(0.5));
        REQUIRE(j["maxDimensions"]["height"] == Approx(1.0));
        REQUIRE(j["minPrice"] == Approx(100.0));
        REQUIRE(j["maxPrice"] == Approx(500.0));
        REQUIRE(j["features"].size() == 2);
        
        CatalogFilter deserializedFilter;
        deserializedFilter.fromJson(j);
        
        REQUIRE(deserializedFilter.searchTerm == "Cabinet");
        REQUIRE(deserializedFilter.categories.size() == 2);
        REQUIRE(deserializedFilter.minDimensions.width == Approx(0.5));
        REQUIRE(deserializedFilter.maxDimensions.height == Approx(1.0));
        REQUIRE(deserializedFilter.minPrice == Approx(100.0));
        REQUIRE(deserializedFilter.maxPrice == Approx(500.0));
        REQUIRE(deserializedFilter.features.size() == 2);
    }
}

TEST_CASE("CatalogSearchResult operations", "[models][catalog][search]") {
    SECTION("Search result creation") {
        // Create test items
        std::vector<std::shared_ptr<CatalogItem>> items;
        for (int i = 0; i < 5; ++i) {
            auto item = std::make_shared<CatalogItem>("item_" + std::to_string(i), 
                                                     "Item " + std::to_string(i), 
                                                     "test_category");
            items.push_back(item);
        }
        
        CatalogSearchResult result(items, 10, 0, 5);
        
        REQUIRE(result.items.size() == 5);
        REQUIRE(result.totalCount == 10);
        REQUIRE(result.offset == 0);
        REQUIRE(result.limit == 5);
        REQUIRE(result.getReturnedCount() == 5);
        REQUIRE(result.hasMore());
        
        // Test result without more items
        CatalogSearchResult completeResult(items, 5, 0, 5);
        REQUIRE_FALSE(completeResult.hasMore());
        
        // Test empty result
        CatalogSearchResult emptyResult;
        REQUIRE(emptyResult.items.empty());
        REQUIRE(emptyResult.totalCount == 0);
        REQUIRE(emptyResult.getReturnedCount() == 0);
        REQUIRE_FALSE(emptyResult.hasMore());
    }
}

TEST_CASE("CatalogItem utility functions", "[models][catalog][utility]") {
    SECTION("ID generation") {
        std::string id1 = CatalogItem::generateId();
        std::string id2 = CatalogItem::generateId();
        
        REQUIRE_FALSE(id1.empty());
        REQUIRE_FALSE(id2.empty());
        REQUIRE(id1 != id2);
        REQUIRE(id1.find("cat_") == 0); // Should start with "cat_"
    }
    
    SECTION("Standard categories") {
        auto categories = CatalogItem::getStandardCategories();
        
        REQUIRE_FALSE(categories.empty());
        REQUIRE(std::find(categories.begin(), categories.end(), "base_cabinets") != categories.end());
        REQUIRE(std::find(categories.begin(), categories.end(), "wall_cabinets") != categories.end());
        REQUIRE(std::find(categories.begin(), categories.end(), "appliances") != categories.end());
        REQUIRE(std::find(categories.begin(), categories.end(), "countertops") != categories.end());
    }
}