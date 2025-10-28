#include "SQLiteProjectRepository.h"
#include "../utils/Logger.h"
#include <nlohmann/json.hpp>
#include <sstream>
#include <iomanip>
#include <fstream>

namespace KitchenCAD {
namespace Persistence {

using json = nlohmann::json;

SQLiteProjectRepository::SQLiteProjectRepository(const std::string& databasePath) {
    db_ = std::make_unique<DatabaseManager>(databasePath);
    loadAutoSaveStatus();
}

SQLiteProjectRepository::SQLiteProjectRepository(std::unique_ptr<DatabaseManager> db)
    : db_(std::move(db)) {
    loadAutoSaveStatus();
}

std::string SQLiteProjectRepository::createProject(const ProjectMetadata& metadata) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!db_ || !db_->isOpen()) {
        LOG_ERROR("Database is not open");
        return "";
    }
    
    // Create new project
    Models::Project project(metadata.name, Models::RoomDimensions(metadata.roomWidth, metadata.roomHeight, metadata.roomDepth));
    project.setDescription(metadata.description);
    
    if (!insertProject(project)) {
        LOG_ERROR("Failed to create project: " + metadata.name);
        return "";
    }
    
    LOG_INFO("Created project: " + project.getName() + " (ID: " + project.getId() + ")");
    return project.getId();
}

std::optional<std::unique_ptr<Models::Project>> SQLiteProjectRepository::loadProject(const std::string& projectId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!db_ || !db_->isOpen()) {
        LOG_ERROR("Database is not open");
        return std::nullopt;
    }
    
    auto project = loadProjectFromDatabase(projectId);
    if (!project) {
        LOG_ERROR("Failed to load project: " + projectId);
        return std::nullopt;
    }
    
    if (!loadSceneObjects(*project)) {
        LOG_WARNING("Failed to load scene objects for project: " + projectId);
    }
    
    LOG_INFO("Loaded project: " + project->getName() + " (ID: " + projectId + ")");
    return project;
}

bool SQLiteProjectRepository::saveProject(const Models::Project& project) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!db_ || !db_->isOpen()) {
        LOG_ERROR("Database is not open");
        return false;
    }
    
    Transaction transaction(*db_);
    
    bool success = false;
    if (projectExists(project.getId())) {
        success = updateProject(project);
    } else {
        success = insertProject(project);
    }
    
    if (!success) {
        LOG_ERROR("Failed to save project: " + project.getName());
        return false;
    }
    
    // Update scene objects
    if (!deleteSceneObjects(project.getId()) || !insertSceneObjects(project)) {
        LOG_ERROR("Failed to save scene objects for project: " + project.getName());
        return false;
    }
    
    // Update object count
    if (!updateProjectObjectCount(project.getId())) {
        LOG_WARNING("Failed to update object count for project: " + project.getName());
    }
    
    if (!transaction.commit()) {
        LOG_ERROR("Failed to commit project save transaction");
        return false;
    }
    
    LOG_INFO("Saved project: " + project.getName() + " (ID: " + project.getId() + ")");
    return true;
}

bool SQLiteProjectRepository::deleteProject(const std::string& projectId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!db_ || !db_->isOpen()) {
        LOG_ERROR("Database is not open");
        return false;
    }
    
    Transaction transaction(*db_);
    
    // Delete project (cascade will handle scene objects and auto-save config)
    auto stmt = db_->prepare("DELETE FROM projects WHERE id = ?");
    if (!stmt || !stmt->isValid()) {
        LOG_ERROR("Failed to prepare delete project statement");
        return false;
    }
    
    stmt->bindText(1, projectId);
    if (!stmt->step()) {
        LOG_ERROR("Failed to delete project: " + projectId);
        return false;
    }
    
    if (db_->getChanges() == 0) {
        LOG_WARNING("Project not found for deletion: " + projectId);
        return false;
    }
    
    if (!transaction.commit()) {
        LOG_ERROR("Failed to commit project deletion");
        return false;
    }
    
    // Remove from auto-save status
    autoSaveStatus_.erase(projectId);
    
    LOG_INFO("Deleted project: " + projectId);
    return true;
}

std::vector<ProjectInfo> SQLiteProjectRepository::listProjects() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<ProjectInfo> projects;
    
    if (!db_ || !db_->isOpen()) {
        LOG_ERROR("Database is not open");
        return projects;
    }
    
    auto stmt = db_->prepare(R"(
        SELECT id, name, description, created_at, updated_at, 
               room_width, room_height, room_depth, object_count, thumbnail_path
        FROM projects 
        ORDER BY updated_at DESC
    )");
    
    if (!stmt || !stmt->isValid()) {
        LOG_ERROR("Failed to prepare list projects statement");
        return projects;
    }
    
    while (stmt->step()) {
        ProjectInfo info;
        info.id = stmt->getColumnText(0);
        info.name = stmt->getColumnText(1);
        info.description = stmt->getColumnText(2);
        info.createdAt = stmt->getColumnText(3);
        info.updatedAt = stmt->getColumnText(4);
        info.roomWidth = stmt->getColumnDouble(5);
        info.roomHeight = stmt->getColumnDouble(6);
        info.roomDepth = stmt->getColumnDouble(7);
        info.objectCount = stmt->getColumnInt(8);
        info.thumbnailPath = stmt->getColumnText(9);
        
        projects.push_back(info);
    }
    
    return projects;
}

std::vector<ProjectInfo> SQLiteProjectRepository::listRecentProjects(size_t maxCount) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<ProjectInfo> projects;
    
    if (!db_ || !db_->isOpen()) {
        LOG_ERROR("Database is not open");
        return projects;
    }
    
    auto stmt = db_->prepare(R"(
        SELECT id, name, description, created_at, updated_at, 
               room_width, room_height, room_depth, object_count, thumbnail_path
        FROM projects 
        ORDER BY updated_at DESC 
        LIMIT ?
    )");
    
    if (!stmt || !stmt->isValid()) {
        LOG_ERROR("Failed to prepare list recent projects statement");
        return projects;
    }
    
    stmt->bindInt64(1, static_cast<int64_t>(maxCount));
    
    while (stmt->step()) {
        ProjectInfo info;
        info.id = stmt->getColumnText(0);
        info.name = stmt->getColumnText(1);
        info.description = stmt->getColumnText(2);
        info.createdAt = stmt->getColumnText(3);
        info.updatedAt = stmt->getColumnText(4);
        info.roomWidth = stmt->getColumnDouble(5);
        info.roomHeight = stmt->getColumnDouble(6);
        info.roomDepth = stmt->getColumnDouble(7);
        info.objectCount = stmt->getColumnInt(8);
        info.thumbnailPath = stmt->getColumnText(9);
        
        projects.push_back(info);
    }
    
    return projects;
}

std::vector<ProjectInfo> SQLiteProjectRepository::searchProjects(const std::string& searchTerm) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<ProjectInfo> projects;
    
    if (!db_ || !db_->isOpen()) {
        LOG_ERROR("Database is not open");
        return projects;
    }
    
    auto stmt = db_->prepare(R"(
        SELECT id, name, description, created_at, updated_at, 
               room_width, room_height, room_depth, object_count, thumbnail_path
        FROM projects 
        WHERE name LIKE ? OR description LIKE ?
        ORDER BY updated_at DESC
    )");
    
    if (!stmt || !stmt->isValid()) {
        LOG_ERROR("Failed to prepare search projects statement");
        return projects;
    }
    
    std::string searchPattern = "%" + searchTerm + "%";
    stmt->bindText(1, searchPattern);
    stmt->bindText(2, searchPattern);
    
    while (stmt->step()) {
        ProjectInfo info;
        info.id = stmt->getColumnText(0);
        info.name = stmt->getColumnText(1);
        info.description = stmt->getColumnText(2);
        info.createdAt = stmt->getColumnText(3);
        info.updatedAt = stmt->getColumnText(4);
        info.roomWidth = stmt->getColumnDouble(5);
        info.roomHeight = stmt->getColumnDouble(6);
        info.roomDepth = stmt->getColumnDouble(7);
        info.objectCount = stmt->getColumnInt(8);
        info.thumbnailPath = stmt->getColumnText(9);
        
        projects.push_back(info);
    }
    
    return projects;
}

std::optional<ProjectInfo> SQLiteProjectRepository::getProjectInfo(const std::string& projectId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!db_ || !db_->isOpen()) {
        LOG_ERROR("Database is not open");
        return std::nullopt;
    }
    
    auto stmt = db_->prepare(R"(
        SELECT id, name, description, created_at, updated_at, 
               room_width, room_height, room_depth, object_count, thumbnail_path
        FROM projects 
        WHERE id = ?
    )");
    
    if (!stmt || !stmt->isValid()) {
        LOG_ERROR("Failed to prepare get project info statement");
        return std::nullopt;
    }
    
    stmt->bindText(1, projectId);
    
    if (stmt->step()) {
        ProjectInfo info;
        info.id = stmt->getColumnText(0);
        info.name = stmt->getColumnText(1);
        info.description = stmt->getColumnText(2);
        info.createdAt = stmt->getColumnText(3);
        info.updatedAt = stmt->getColumnText(4);
        info.roomWidth = stmt->getColumnDouble(5);
        info.roomHeight = stmt->getColumnDouble(6);
        info.roomDepth = stmt->getColumnDouble(7);
        info.objectCount = stmt->getColumnInt(8);
        info.thumbnailPath = stmt->getColumnText(9);
        
        return info;
    }
    
    return std::nullopt;
}

bool SQLiteProjectRepository::updateProjectMetadata(const std::string& projectId, const ProjectMetadata& metadata) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!db_ || !db_->isOpen()) {
        LOG_ERROR("Database is not open");
        return false;
    }
    
    auto stmt = db_->prepare(R"(
        UPDATE projects 
        SET name = ?, description = ?, room_width = ?, room_height = ?, room_depth = ?, updated_at = datetime('now')
        WHERE id = ?
    )");
    
    if (!stmt || !stmt->isValid()) {
        LOG_ERROR("Failed to prepare update project metadata statement");
        return false;
    }
    
    stmt->bindText(1, metadata.name);
    stmt->bindText(2, metadata.description);
    stmt->bindDouble(3, metadata.roomWidth);
    stmt->bindDouble(4, metadata.roomHeight);
    stmt->bindDouble(5, metadata.roomDepth);
    stmt->bindText(6, projectId);
    
    if (!stmt->step()) {
        LOG_ERROR("Failed to update project metadata: " + projectId);
        return false;
    }
    
    return db_->getChanges() > 0;
}

bool SQLiteProjectRepository::setProjectThumbnail(const std::string& projectId, const std::string& thumbnailPath) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!db_ || !db_->isOpen()) {
        LOG_ERROR("Database is not open");
        return false;
    }
    
    auto stmt = db_->prepare("UPDATE projects SET thumbnail_path = ?, updated_at = datetime('now') WHERE id = ?");
    if (!stmt || !stmt->isValid()) {
        LOG_ERROR("Failed to prepare set thumbnail statement");
        return false;
    }
    
    stmt->bindText(1, thumbnailPath);
    stmt->bindText(2, projectId);
    
    if (!stmt->step()) {
        LOG_ERROR("Failed to set project thumbnail: " + projectId);
        return false;
    }
    
    return db_->getChanges() > 0;
}

bool SQLiteProjectRepository::projectExists(const std::string& projectId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!db_ || !db_->isOpen()) {
        return false;
    }
    
    auto stmt = db_->prepare("SELECT 1 FROM projects WHERE id = ?");
    if (!stmt || !stmt->isValid()) {
        return false;
    }
    
    stmt->bindText(1, projectId);
    return stmt->step();
}

bool SQLiteProjectRepository::isValidProjectName(const std::string& name) {
    if (name.empty() || name.length() > 255) {
        return false;
    }
    
    // Check for invalid characters
    const std::string invalidChars = "<>:\"/\\|?*";
    return name.find_first_of(invalidChars) == std::string::npos;
}

bool SQLiteProjectRepository::exportProject(const std::string& projectId, const std::string& filePath) {
    auto project = loadProject(projectId);
    if (!project) {
        LOG_ERROR("Failed to load project for export: " + projectId);
        return false;
    }
    
    try {
        json exportData = (*project)->toJson();
        
        std::ofstream file(filePath);
        if (!file.is_open()) {
            LOG_ERROR("Failed to open export file: " + filePath);
            return false;
        }
        
        file << exportData.dump(2);
        file.close();
        
        LOG_INFO("Exported project to: " + filePath);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to export project: " + std::string(e.what()));
        return false;
    }
}

std::optional<std::string> SQLiteProjectRepository::importProject(const std::string& filePath) {
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            LOG_ERROR("Failed to open import file: " + filePath);
            return std::nullopt;
        }
        
        json importData;
        file >> importData;
        file.close();
        
        auto project = std::make_unique<Models::Project>();
        project->fromJson(importData);
        
        // Generate new ID to avoid conflicts
        std::string originalId = project->getId();
        project->setId(Models::Project::generateId());
        
        if (!saveProject(*project)) {
            LOG_ERROR("Failed to save imported project");
            return std::nullopt;
        }
        
        LOG_INFO("Imported project from: " + filePath + " (New ID: " + project->getId() + ")");
        return project->getId();
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to import project: " + std::string(e.what()));
        return std::nullopt;
    }
}

bool SQLiteProjectRepository::vacuum() {
    std::lock_guard<std::mutex> lock(mutex_);
    return db_ && db_->vacuum();
}

bool SQLiteProjectRepository::backup(const std::string& backupPath) {
    std::lock_guard<std::mutex> lock(mutex_);
    return db_ && db_->backup(backupPath);
}

bool SQLiteProjectRepository::restore(const std::string& backupPath) {
    std::lock_guard<std::mutex> lock(mutex_);
    bool result = db_ && db_->restore(backupPath);
    if (result) {
        loadAutoSaveStatus();
    }
    return result;
}

size_t SQLiteProjectRepository::getTotalProjectCount() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!db_ || !db_->isOpen()) {
        return 0;
    }
    
    auto stmt = db_->prepare("SELECT COUNT(*) FROM projects");
    if (!stmt || !stmt->isValid()) {
        return 0;
    }
    
    if (stmt->step()) {
        return static_cast<size_t>(stmt->getColumnInt64(0));
    }
    
    return 0;
}

size_t SQLiteProjectRepository::getDatabaseSize() {
    std::lock_guard<std::mutex> lock(mutex_);
    return db_ ? static_cast<size_t>(db_->getDatabaseSize()) : 0;
}

bool SQLiteProjectRepository::enableAutoSave(const std::string& projectId, int intervalSeconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!updateAutoSaveConfig(projectId, true, intervalSeconds)) {
        return false;
    }
    
    autoSaveStatus_[projectId] = true;
    return true;
}

bool SQLiteProjectRepository::disableAutoSave(const std::string& projectId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!updateAutoSaveConfig(projectId, false, 30)) {
        return false;
    }
    
    autoSaveStatus_[projectId] = false;
    return true;
}

bool SQLiteProjectRepository::isAutoSaveEnabled(const std::string& projectId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = autoSaveStatus_.find(projectId);
    return it != autoSaveStatus_.end() && it->second;
}

// Private helper methods
bool SQLiteProjectRepository::insertProject(const Models::Project& project) {
    auto stmt = db_->prepare(R"(
        INSERT INTO projects (id, name, description, room_width, room_height, room_depth, 
                             scene_data, metadata, thumbnail_path, object_count)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");
    
    if (!stmt || !stmt->isValid()) {
        LOG_ERROR("Failed to prepare insert project statement");
        return false;
    }
    
    json sceneData = project.serializeSceneToJson();
    json metadata = project.toProjectMetadata().name; // Simplified metadata
    
    stmt->bindText(1, project.getId());
    stmt->bindText(2, project.getName());
    stmt->bindText(3, project.getDescription());
    stmt->bindDouble(4, project.getDimensions().width);
    stmt->bindDouble(5, project.getDimensions().height);
    stmt->bindDouble(6, project.getDimensions().depth);
    stmt->bindText(7, sceneData.dump());
    stmt->bindText(8, metadata);
    stmt->bindText(9, project.getThumbnailPath());
    stmt->bindInt64(10, static_cast<int64_t>(project.getObjectCount()));
    
    return stmt->step();
}

bool SQLiteProjectRepository::updateProject(const Models::Project& project) {
    auto stmt = db_->prepare(R"(
        UPDATE projects 
        SET name = ?, description = ?, room_width = ?, room_height = ?, room_depth = ?, 
            scene_data = ?, metadata = ?, thumbnail_path = ?, object_count = ?, updated_at = datetime('now')
        WHERE id = ?
    )");
    
    if (!stmt || !stmt->isValid()) {
        LOG_ERROR("Failed to prepare update project statement");
        return false;
    }
    
    json sceneData = project.serializeSceneToJson();
    json metadata = project.toProjectMetadata().name; // Simplified metadata
    
    stmt->bindText(1, project.getName());
    stmt->bindText(2, project.getDescription());
    stmt->bindDouble(3, project.getDimensions().width);
    stmt->bindDouble(4, project.getDimensions().height);
    stmt->bindDouble(5, project.getDimensions().depth);
    stmt->bindText(6, sceneData.dump());
    stmt->bindText(7, metadata);
    stmt->bindText(8, project.getThumbnailPath());
    stmt->bindInt64(9, static_cast<int64_t>(project.getObjectCount()));
    stmt->bindText(10, project.getId());
    
    return stmt->step();
}

bool SQLiteProjectRepository::insertSceneObjects(const Models::Project& project) {
    auto stmt = db_->prepare(R"(
        INSERT INTO scene_objects (id, project_id, catalog_item_id, position_x, position_y, position_z,
                                  rotation_x, rotation_y, rotation_z, scale_x, scale_y, scale_z,
                                  material_id, custom_properties)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");
    
    if (!stmt || !stmt->isValid()) {
        LOG_ERROR("Failed to prepare insert scene objects statement");
        return false;
    }
    
    for (const auto& object : project.getObjects()) {
        const auto& transform = object->getTransform();
        const auto& material = object->getMaterial();
        
        stmt->reset();
        stmt->bindText(1, object->getId());
        stmt->bindText(2, project.getId());
        stmt->bindText(3, object->getCatalogItemId());
        stmt->bindDouble(4, transform.translation.x);
        stmt->bindDouble(5, transform.translation.y);
        stmt->bindDouble(6, transform.translation.z);
        stmt->bindDouble(7, transform.rotation.x);
        stmt->bindDouble(8, transform.rotation.y);
        stmt->bindDouble(9, transform.rotation.z);
        stmt->bindDouble(10, transform.scale.x);
        stmt->bindDouble(11, transform.scale.y);
        stmt->bindDouble(12, transform.scale.z);
        stmt->bindText(13, material.id);
        stmt->bindText(14, object->getCustomProperties());
        
        if (!stmt->step()) {
            LOG_ERROR("Failed to insert scene object: " + object->getId());
            return false;
        }
    }
    
    return true;
}

bool SQLiteProjectRepository::deleteSceneObjects(const std::string& projectId) {
    auto stmt = db_->prepare("DELETE FROM scene_objects WHERE project_id = ?");
    if (!stmt || !stmt->isValid()) {
        LOG_ERROR("Failed to prepare delete scene objects statement");
        return false;
    }
    
    stmt->bindText(1, projectId);
    return stmt->step();
}

std::unique_ptr<Models::Project> SQLiteProjectRepository::loadProjectFromDatabase(const std::string& projectId) {
    auto stmt = db_->prepare(R"(
        SELECT id, name, description, room_width, room_height, room_depth, 
               scene_data, metadata, thumbnail_path, created_at, updated_at
        FROM projects 
        WHERE id = ?
    )");
    
    if (!stmt || !stmt->isValid()) {
        LOG_ERROR("Failed to prepare load project statement");
        return nullptr;
    }
    
    stmt->bindText(1, projectId);
    
    if (!stmt->step()) {
        return nullptr;
    }
    
    auto project = std::make_unique<Models::Project>();
    project->setId(stmt->getColumnText(0));
    project->setName(stmt->getColumnText(1));
    project->setDescription(stmt->getColumnText(2));
    
    Models::RoomDimensions dimensions(
        stmt->getColumnDouble(3),
        stmt->getColumnDouble(4),
        stmt->getColumnDouble(5)
    );
    project->setDimensions(dimensions);
    
    project->setThumbnailPath(stmt->getColumnText(8));
    
    // Load scene data if available
    std::string sceneDataStr = stmt->getColumnText(6);
    if (!sceneDataStr.empty()) {
        try {
            json sceneData = json::parse(sceneDataStr);
            project->loadSceneFromJson(sceneData);
        } catch (const std::exception& e) {
            LOG_WARNING("Failed to parse scene data for project " + projectId + ": " + e.what());
        }
    }
    
    return project;
}

bool SQLiteProjectRepository::loadSceneObjects(Models::Project& project) {
    auto stmt = db_->prepare(R"(
        SELECT id, catalog_item_id, position_x, position_y, position_z,
               rotation_x, rotation_y, rotation_z, scale_x, scale_y, scale_z,
               material_id, custom_properties
        FROM scene_objects 
        WHERE project_id = ?
    )");
    
    if (!stmt || !stmt->isValid()) {
        LOG_ERROR("Failed to prepare load scene objects statement");
        return false;
    }
    
    stmt->bindText(1, project.getId());
    
    while (stmt->step()) {
        auto object = std::make_unique<Models::SceneObject>(stmt->getColumnText(1));
        object->setId(stmt->getColumnText(0));
        
        // Set transform
        Models::Transform3D transform;
        transform.translation = Models::Point3D(
            stmt->getColumnDouble(2),
            stmt->getColumnDouble(3),
            stmt->getColumnDouble(4)
        );
        transform.rotation = Models::Vector3D(
            stmt->getColumnDouble(5),
            stmt->getColumnDouble(6),
            stmt->getColumnDouble(7)
        );
        transform.scale = Models::Vector3D(
            stmt->getColumnDouble(8),
            stmt->getColumnDouble(9),
            stmt->getColumnDouble(10)
        );
        object->setTransform(transform);
        
        // Set material
        Models::MaterialProperties material;
        material.id = stmt->getColumnText(11);
        object->setMaterial(material);
        
        object->setCustomProperties(stmt->getColumnText(12));
        
        project.addObject(std::move(object));
    }
    
    return true;
}

bool SQLiteProjectRepository::updateProjectObjectCount(const std::string& projectId) {
    auto stmt = db_->prepare(R"(
        UPDATE projects 
        SET object_count = (SELECT COUNT(*) FROM scene_objects WHERE project_id = ?)
        WHERE id = ?
    )");
    
    if (!stmt || !stmt->isValid()) {
        return false;
    }
    
    stmt->bindText(1, projectId);
    stmt->bindText(2, projectId);
    
    return stmt->step();
}

bool SQLiteProjectRepository::updateAutoSaveConfig(const std::string& projectId, bool enabled, int intervalSeconds) {
    if (!db_ || !db_->isOpen()) {
        return false;
    }
    
    auto stmt = db_->prepare(R"(
        INSERT OR REPLACE INTO auto_save_config (project_id, enabled, interval_seconds, last_save)
        VALUES (?, ?, ?, datetime('now'))
    )");
    
    if (!stmt || !stmt->isValid()) {
        return false;
    }
    
    stmt->bindText(1, projectId);
    stmt->bindInt(2, enabled ? 1 : 0);
    stmt->bindInt(3, intervalSeconds);
    
    return stmt->step();
}

bool SQLiteProjectRepository::loadAutoSaveStatus() {
    if (!db_ || !db_->isOpen()) {
        return false;
    }
    
    auto stmt = db_->prepare("SELECT project_id, enabled FROM auto_save_config");
    if (!stmt || !stmt->isValid()) {
        return false;
    }
    
    autoSaveStatus_.clear();
    while (stmt->step()) {
        std::string projectId = stmt->getColumnText(0);
        bool enabled = stmt->getColumnInt(1) != 0;
        autoSaveStatus_[projectId] = enabled;
    }
    
    return true;
}

} // namespace Persistence
} // namespace KitchenCAD