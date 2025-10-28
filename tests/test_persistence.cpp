#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../src/persistence/DatabaseManager.h"
#include "../src/persistence/SQLiteProjectRepository.h"
#include "../src/persistence/CatalogRepository.h"
#include "../src/models/Project.h"
#include "../src/models/CatalogItem.h"
#include <filesystem>
#include <memory>

using namespace KitchenCAD;
using namespace KitchenCAD::Persistence;
using namespace KitchenCAD::Models;
using Catch::Approx;

// Test database path
const std::string TEST_DB_PATH = "test_kitchen_cad.db";

// Helper function to clean up test database
void cleanupTestDatabase() {
    if (std::filesystem::exists(TEST_DB_PATH)) {
        std::filesystem::remove(TEST_DB_PATH);
    }
}

TEST_CASE("DatabaseManager basic operations", "[persistence][database]") {
    cleanupTestDatabase();
    
    SECTION("Database creation and opening") {
        DatabaseManager db;
        REQUIRE(db.open(TEST_DB_PATH));
        REQUIRE(db.isOpen());
        REQUIRE(db.getPath() == TEST_DB_PATH);
        
        db.close();
        REQUIRE(!db.isOpen());
    }
    
    SECTION("SQL execution") {
        DatabaseManager db(TEST_DB_PATH);
        REQUIRE(db.isOpen());
        
        // Create a test table
        REQUIRE(db.execute("CREATE TABLE test_table (id INTEGER PRIMARY KEY, name TEXT)"));
        
        // Insert data
        REQUIRE(db.execute("INSERT INTO test_table (name) VALUES ('test')"));
        REQUIRE(db.getLastInsertRowId() > 0);
        REQUIRE(db.getChanges() == 1);
        
        // Query data
        auto stmt = db.prepare("SELECT id, name FROM test_table WHERE name = ?");
        REQUIRE(stmt != nullptr);
        REQUIRE(stmt->isValid());
        
        stmt->bindText(1, "test");
        REQUIRE(stmt->step());
        
        REQUIRE(stmt->getColumnInt(0) > 0);
        REQUIRE(stmt->getColumnText(1) == "test");
    }
    
    SECTION("Transaction management") {
        DatabaseManager db(TEST_DB_PATH);
        REQUIRE(db.isOpen());
        
        // Create test table
        REQUIRE(db.execute("CREATE TABLE test_table (id INTEGER PRIMARY KEY, value INTEGER)"));
        
        // Test successful transaction
        {
            Transaction transaction(db);
            REQUIRE(db.execute("INSERT INTO test_table (value) VALUES (1)"));
            REQUIRE(db.execute("INSERT INTO test_table (value) VALUES (2)"));
            REQUIRE(transaction.commit());
        }
        
        // Verify data was committed
        auto stmt = db.prepare("SELECT COUNT(*) FROM test_table");
        REQUIRE(stmt->step());
        REQUIRE(stmt->getColumnInt(0) == 2);
        
        // Test rollback transaction
        {
            Transaction transaction(db);
            REQUIRE(db.execute("INSERT INTO test_table (value) VALUES (3)"));
            // Don't commit - should rollback automatically
        }
        
        // Verify data was rolled back
        stmt->reset();
        REQUIRE(stmt->step());
        REQUIRE(stmt->getColumnInt(0) == 2);
    }
    
    SECTION("Migration system") {
        DatabaseManager db(TEST_DB_PATH);
        REQUIRE(db.isOpen());
        
        // Add a test migration
        Migration testMigration(100, "Test migration", {
            "CREATE TABLE test_migration (id INTEGER PRIMARY KEY, data TEXT)"
        });
        
        db.addMigration(testMigration);
        REQUIRE(db.runMigrations());
        
        // Verify migration was applied
        REQUIRE(db.getCurrentVersion() >= 100);
        
        // Verify table was created
        auto stmt = db.prepare("SELECT name FROM sqlite_master WHERE type='table' AND name='test_migration'");
        REQUIRE(stmt->step());
        REQUIRE(stmt->getColumnText(0) == "test_migration");
    }
    
    cleanupTestDatabase();
}

TEST_CASE("Project model operations", "[persistence][project]") {
    SECTION("Project creation and serialization") {
        RoomDimensions dimensions(5.0, 3.0, 2.5);
        Project project("Test Kitchen", dimensions);
        
        REQUIRE(project.getName() == "Test Kitchen");
        REQUIRE(project.getDimensions().width == Approx(5.0));
        REQUIRE(project.getDimensions().height == Approx(3.0));
        REQUIRE(project.getDimensions().depth == Approx(2.5));
        REQUIRE(project.getObjectCount() == 0);
        
        // Add some objects
        auto object1 = std::make_unique<SceneObject>("catalog_item_1");
        auto object2 = std::make_unique<SceneObject>("catalog_item_2");
        
        std::string obj1Id = project.addObject(std::move(object1));
        std::string obj2Id = project.addObject(std::move(object2));
        
        REQUIRE(!obj1Id.empty());
        REQUIRE(!obj2Id.empty());
        REQUIRE(project.getObjectCount() == 2);
        
        // Test JSON serialization
        auto json = project.toJson();
        REQUIRE(!json.empty());
        
        // Create new project from JSON
        Project deserializedProject;
        deserializedProject.fromJson(json);
        
        REQUIRE(deserializedProject.getName() == project.getName());
        REQUIRE(deserializedProject.getDimensions().width == Approx(project.getDimensions().width));
        REQUIRE(deserializedProject.getObjectCount() == project.getObjectCount());
    }
    
    SECTION("Project validation") {
        Project project("Valid Project", RoomDimensions(5.0, 3.0, 2.5));
        REQUIRE(project.isValid());
        
        // Test invalid project
        Project invalidProject("", RoomDimensions(0.0, 0.0, 0.0));
        REQUIRE(!invalidProject.isValid());
        
        auto errors = invalidProject.validate();
        REQUIRE(!errors.empty());
    }
}

TEST_CASE("SQLiteProjectRepository operations", "[persistence][repository]") {
    cleanupTestDatabase();
    
    SECTION("Project CRUD operations") {
        auto repository = std::make_unique<SQLiteProjectRepository>(TEST_DB_PATH);
        REQUIRE(repository->isConnected());
        
        // Create project
        ProjectMetadata metadata("Test Kitchen", 5.0, 3.0, 2.5);
        metadata.description = "A test kitchen design";
        
        std::string projectId = repository->createProject(metadata);
        REQUIRE(!projectId.empty());
        REQUIRE(repository->projectExists(projectId));
        
        // Load project
        auto loadedProject = repository->loadProject(projectId);
        REQUIRE(loadedProject.has_value());
        REQUIRE((*loadedProject)->getName() == "Test Kitchen");
        REQUIRE((*loadedProject)->getDimensions().width == Approx(5.0));
        
        // Update project
        (*loadedProject)->setDescription("Updated description");
        auto object = std::make_unique<SceneObject>("test_catalog_item");
        (*loadedProject)->addObject(std::move(object));
        
        REQUIRE(repository->saveProject(**loadedProject));
        
        // Verify update
        auto updatedProject = repository->loadProject(projectId);
        REQUIRE(updatedProject.has_value());
        REQUIRE((*updatedProject)->getDescription() == "Updated description");
        REQUIRE((*updatedProject)->getObjectCount() == 1);
        
        // Delete project
        REQUIRE(repository->deleteProject(projectId));
        REQUIRE(!repository->projectExists(projectId));
    }
    
    SECTION("Project queries") {
        auto repository = std::make_unique<SQLiteProjectRepository>(TEST_DB_PATH);
        
        // Create multiple projects
        std::vector<std::string> projectIds;
        for (int i = 0; i < 3; ++i) {
            ProjectMetadata metadata("Project " + std::to_string(i), 5.0, 3.0, 2.5);
            std::string id = repository->createProject(metadata);
            REQUIRE(!id.empty());
            projectIds.push_back(id);
        }
        
        // List all projects
        auto allProjects = repository->listProjects();
        REQUIRE(allProjects.size() >= 3);
        
        // List recent projects
        auto recentProjects = repository->listRecentProjects(2);
        REQUIRE(recentProjects.size() == 2);
        
        // Search projects
        auto searchResults = repository->searchProjects("Project");
        REQUIRE(searchResults.size() >= 3);
        
        // Get project info
        auto projectInfo = repository->getProjectInfo(projectIds[0]);
        REQUIRE(projectInfo.has_value());
        REQUIRE(projectInfo->name == "Project 0");
        
        // Clean up
        for (const auto& id : projectIds) {
            repository->deleteProject(id);
        }
    }
    
    SECTION("Auto-save functionality") {
        auto repository = std::make_unique<SQLiteProjectRepository>(TEST_DB_PATH);
        
        // Create project
        ProjectMetadata metadata("Auto-save Test", 5.0, 3.0, 2.5);
        std::string projectId = repository->createProject(metadata);
        REQUIRE(!projectId.empty());
        
        // Test auto-save configuration
        REQUIRE(!repository->isAutoSaveEnabled(projectId));
        REQUIRE(repository->enableAutoSave(projectId, 30));
        REQUIRE(repository->isAutoSaveEnabled(projectId));
        REQUIRE(repository->disableAutoSave(projectId));
        REQUIRE(!repository->isAutoSaveEnabled(projectId));
        
        // Clean up
        repository->deleteProject(projectId);
    }
    
    cleanupTestDatabase();
}

TEST_CASE("CatalogItem model operations", "[persistence][catalog]") {
    SECTION("CatalogItem creation and validation") {
        CatalogItem item("test_item", "Test Cabinet", "base_cabinets");
        item.setDimensions(Dimensions3D(0.6, 0.85, 0.6));
        item.setBasePrice(299.99);
        
        REQUIRE(item.getName() == "Test Cabinet");
        REQUIRE(item.getCategory() == "base_cabinets");
        REQUIRE(item.getDimensions().width == Approx(0.6));
        REQUIRE(item.getBasePrice() == Approx(299.99));
        REQUIRE(item.isValid());
        
        // Add material option
        MaterialOption option("wood_oak", "Oak Wood", 50.0);
        item.addMaterialOption(option);
        
        REQUIRE(item.getMaterialOptions().size() == 1);
        REQUIRE(item.getPrice("wood_oak") == Approx(349.99));
        
        // Test JSON serialization
        auto json = item.toJson();
        REQUIRE(!json.empty());
        
        CatalogItem deserializedItem;
        deserializedItem.fromJson(json);
        
        REQUIRE(deserializedItem.getName() == item.getName());
        REQUIRE(deserializedItem.getBasePrice() == Approx(item.getBasePrice()));
        REQUIRE(deserializedItem.getMaterialOptions().size() == 1);
    }
    
    SECTION("CatalogFilter operations") {
        CatalogItem item1("item1", "Base Cabinet 60", "base_cabinets");
        item1.setDimensions(Dimensions3D(0.6, 0.85, 0.6));
        item1.setBasePrice(299.99);
        
        CatalogItem item2("item2", "Wall Cabinet 80", "wall_cabinets");
        item2.setDimensions(Dimensions3D(0.8, 0.7, 0.35));
        item2.setBasePrice(199.99);
        
        // Test search filter
        CatalogFilter filter;
        filter.searchTerm = "Cabinet";
        REQUIRE(filter.matches(item1));
        REQUIRE(filter.matches(item2));
        
        filter.searchTerm = "Base";
        REQUIRE(filter.matches(item1));
        REQUIRE(!filter.matches(item2));
        
        // Test category filter
        filter.searchTerm = "";
        filter.categories = {"base_cabinets"};
        REQUIRE(filter.matches(item1));
        REQUIRE(!filter.matches(item2));
        
        // Test price filter
        filter.categories.clear();
        filter.minPrice = 250.0;
        filter.maxPrice = 350.0;
        REQUIRE(filter.matches(item1));
        REQUIRE(!filter.matches(item2));
    }
}

TEST_CASE("SQLiteCatalogRepository operations", "[persistence][catalog][repository]") {
    cleanupTestDatabase();
    
    SECTION("Catalog item CRUD operations") {
        auto repository = std::make_unique<SQLiteCatalogRepository>(TEST_DB_PATH);
        REQUIRE(repository->isConnected());
        
        // Create catalog item
        CatalogItem item("test_cabinet", "Test Base Cabinet", "base_cabinets");
        item.setDimensions(Dimensions3D(0.6, 0.85, 0.6));
        item.setBasePrice(299.99);
        
        // Add material options
        MaterialOption option1("wood_oak", "Oak Wood", 50.0);
        MaterialOption option2("wood_maple", "Maple Wood", 75.0);
        item.addMaterialOption(option1);
        item.addMaterialOption(option2);
        
        REQUIRE(repository->addItem(item));
        REQUIRE(repository->itemExists("test_cabinet"));
        
        // Load item
        auto loadedItem = repository->getItem("test_cabinet");
        REQUIRE(loadedItem.has_value());
        REQUIRE(loadedItem->getName() == "Test Base Cabinet");
        REQUIRE(loadedItem->getMaterialOptions().size() == 2);
        
        // Update item
        loadedItem->setBasePrice(349.99);
        REQUIRE(repository->updateItem(*loadedItem));
        
        // Verify update
        auto updatedItem = repository->getItem("test_cabinet");
        REQUIRE(updatedItem.has_value());
        REQUIRE(updatedItem->getBasePrice() == Approx(349.99));
        
        // Delete item
        REQUIRE(repository->deleteItem("test_cabinet"));
        REQUIRE(!repository->itemExists("test_cabinet"));
    }
    
    SECTION("Catalog queries and search") {
        auto repository = std::make_unique<SQLiteCatalogRepository>(TEST_DB_PATH);
        
        // Create multiple items
        std::vector<CatalogItem> items;
        
        CatalogItem item1("base_60", "Base Cabinet 60cm", "base_cabinets");
        item1.setDimensions(Dimensions3D(0.6, 0.85, 0.6));
        item1.setBasePrice(299.99);
        items.push_back(item1);
        
        CatalogItem item2("wall_80", "Wall Cabinet 80cm", "wall_cabinets");
        item2.setDimensions(Dimensions3D(0.8, 0.7, 0.35));
        item2.setBasePrice(199.99);
        items.push_back(item2);
        
        CatalogItem item3("tall_60", "Tall Cabinet 60cm", "tall_cabinets");
        item3.setDimensions(Dimensions3D(0.6, 2.0, 0.6));
        item3.setBasePrice(599.99);
        items.push_back(item3);
        
        // Add items to repository
        for (const auto& item : items) {
            REQUIRE(repository->addItem(item));
        }
        
        // Test get all items
        auto allItems = repository->getAllItems();
        REQUIRE(allItems.size() >= 3);
        
        // Test get by category
        auto baseItems = repository->getItemsByCategory("base_cabinets");
        REQUIRE(baseItems.size() >= 1);
        REQUIRE(baseItems[0].getCategory() == "base_cabinets");
        
        // Test search
        CatalogFilter filter;
        filter.searchTerm = "Cabinet";
        auto searchResult = repository->searchItems(filter);
        REQUIRE(searchResult.items.size() >= 3);
        
        filter.categories = {"wall_cabinets"};
        searchResult = repository->searchItems(filter);
        REQUIRE(searchResult.items.size() >= 1);
        
        // Test get categories
        auto categories = repository->getCategories();
        REQUIRE(categories.size() >= 3);
        
        // Test statistics
        REQUIRE(repository->getItemCount() >= 3);
        REQUIRE(repository->getItemCountByCategory("base_cabinets") >= 1);
        
        // Clean up
        for (const auto& item : items) {
            repository->deleteItem(item.getId());
        }
    }
    
    SECTION("Material options management") {
        auto repository = std::make_unique<SQLiteCatalogRepository>(TEST_DB_PATH);
        
        // Create item
        CatalogItem item("test_item", "Test Item", "test_category");
        item.setDimensions(Dimensions3D(1.0, 1.0, 1.0));
        item.setBasePrice(100.0);
        REQUIRE(repository->addItem(item));
        
        // Add material options
        MaterialOption option1("mat1", "Material 1", 10.0);
        MaterialOption option2("mat2", "Material 2", 20.0);
        
        REQUIRE(repository->addMaterialOption("test_item", option1));
        REQUIRE(repository->addMaterialOption("test_item", option2));
        
        // Get material options
        auto options = repository->getMaterialOptions("test_item");
        REQUIRE(options.size() == 2);
        
        // Update material option
        option1.priceModifier = 15.0;
        REQUIRE(repository->updateMaterialOption(option1));
        
        // Delete material option
        REQUIRE(repository->deleteMaterialOption("mat2"));
        
        options = repository->getMaterialOptions("test_item");
        REQUIRE(options.size() == 1);
        REQUIRE(options[0].priceModifier == Approx(15.0));
        
        // Clean up
        repository->deleteItem("test_item");
    }
    
    SECTION("Bulk operations") {
        auto repository = std::make_unique<SQLiteCatalogRepository>(TEST_DB_PATH);
        
        // Create items for bulk import
        std::vector<CatalogItem> items;
        for (int i = 0; i < 5; ++i) {
            CatalogItem item("bulk_" + std::to_string(i), "Bulk Item " + std::to_string(i), "bulk_category");
            item.setDimensions(Dimensions3D(1.0, 1.0, 1.0));
            item.setBasePrice(100.0 + i * 10.0);
            items.push_back(item);
        }
        
        // Test bulk import
        REQUIRE(repository->importCatalog(items));
        REQUIRE(repository->getItemCountByCategory("bulk_category") == 5);
        
        // Test export
        auto exportedItems = repository->exportCatalog();
        REQUIRE(exportedItems.size() >= 5);
        
        // Test clear catalog
        REQUIRE(repository->clearCatalog());
        REQUIRE(repository->getItemCount() == 0);
    }
    
    cleanupTestDatabase();
}

TEST_CASE("Database maintenance operations", "[persistence][maintenance]") {
    cleanupTestDatabase();
    
    SECTION("Backup and restore") {
        const std::string BACKUP_PATH = "test_backup.db";
        
        // Create database with some data
        {
            auto repository = std::make_unique<SQLiteProjectRepository>(TEST_DB_PATH);
            ProjectMetadata metadata("Backup Test", 5.0, 3.0, 2.5);
            std::string projectId = repository->createProject(metadata);
            REQUIRE(!projectId.empty());
        }
        
        // Test backup
        {
            auto repository = std::make_unique<SQLiteProjectRepository>(TEST_DB_PATH);
            REQUIRE(repository->backup(BACKUP_PATH));
            REQUIRE(std::filesystem::exists(BACKUP_PATH));
        }
        
        // Modify original database
        {
            auto repository = std::make_unique<SQLiteProjectRepository>(TEST_DB_PATH);
            ProjectMetadata metadata("Modified Project", 10.0, 6.0, 5.0);
            repository->createProject(metadata);
        }
        
        // Test restore
        {
            auto repository = std::make_unique<SQLiteProjectRepository>(TEST_DB_PATH);
            REQUIRE(repository->restore(BACKUP_PATH));
            
            auto projects = repository->listProjects();
            REQUIRE(projects.size() == 1);
            REQUIRE(projects[0].name == "Backup Test");
        }
        
        // Clean up
        std::filesystem::remove(BACKUP_PATH);
    }
    
    SECTION("Database statistics") {
        auto repository = std::make_unique<SQLiteProjectRepository>(TEST_DB_PATH);
        
        // Create some projects
        for (int i = 0; i < 3; ++i) {
            ProjectMetadata metadata("Project " + std::to_string(i), 5.0, 3.0, 2.5);
            repository->createProject(metadata);
        }
        
        REQUIRE(repository->getTotalProjectCount() == 3);
        REQUIRE(repository->getDatabaseSize() > 0);
        
        // Test vacuum
        REQUIRE(repository->vacuum());
    }
    
    cleanupTestDatabase();
}