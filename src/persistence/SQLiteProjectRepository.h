#pragma once

#include "../interfaces/IProjectRepository.h"
#include "DatabaseManager.h"
#include <memory>
#include <unordered_map>
#include <mutex>

namespace KitchenCAD {
namespace Persistence {

/**
 * @brief SQLite implementation of IProjectRepository
 * 
 * Provides persistent storage for projects using SQLite database
 */
class SQLiteProjectRepository : public IProjectRepository {
private:
    std::unique_ptr<DatabaseManager> db_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, bool> autoSaveStatus_;
    
public:
    explicit SQLiteProjectRepository(const std::string& databasePath);
    explicit SQLiteProjectRepository(std::unique_ptr<DatabaseManager> db);
    ~SQLiteProjectRepository() override = default;
    
    // Non-copyable and non-movable (due to mutex)
    SQLiteProjectRepository(const SQLiteProjectRepository&) = delete;
    SQLiteProjectRepository& operator=(const SQLiteProjectRepository&) = delete;
    SQLiteProjectRepository(SQLiteProjectRepository&&) = delete;
    SQLiteProjectRepository& operator=(SQLiteProjectRepository&&) = delete;
    
    // Project CRUD operations
    std::string createProject(const ProjectMetadata& metadata) override;
    std::optional<std::unique_ptr<Project>> loadProject(const std::string& projectId) override;
    bool saveProject(const Project& project) override;
    bool deleteProject(const std::string& projectId) override;
    
    // Project queries
    std::vector<ProjectInfo> listProjects() override;
    std::vector<ProjectInfo> listRecentProjects(size_t maxCount = 10) override;
    std::vector<ProjectInfo> searchProjects(const std::string& searchTerm) override;
    std::optional<ProjectInfo> getProjectInfo(const std::string& projectId) override;
    
    // Project metadata operations
    bool updateProjectMetadata(const std::string& projectId, const ProjectMetadata& metadata) override;
    bool setProjectThumbnail(const std::string& projectId, const std::string& thumbnailPath) override;
    
    // Project validation
    bool projectExists(const std::string& projectId) override;
    bool isValidProjectName(const std::string& name) override;
    
    // Backup and restore
    bool exportProject(const std::string& projectId, const std::string& filePath) override;
    std::optional<std::string> importProject(const std::string& filePath) override;
    
    // Database maintenance
    bool vacuum() override;
    bool backup(const std::string& backupPath) override;
    bool restore(const std::string& backupPath) override;
    
    // Statistics
    size_t getTotalProjectCount() override;
    size_t getDatabaseSize() override;
    
    // Auto-save support
    bool enableAutoSave(const std::string& projectId, int intervalSeconds = 30) override;
    bool disableAutoSave(const std::string& projectId) override;
    bool isAutoSaveEnabled(const std::string& projectId) override;
    
    // Additional utility methods
    DatabaseManager* getDatabase() const { return db_.get(); }
    bool isConnected() const { return db_ && db_->isOpen(); }
    
private:
    // Helper methods for database operations
    bool insertProject(const Project& project);
    bool updateProject(const Project& project);
    bool insertSceneObjects(const Project& project);
    bool deleteSceneObjects(const std::string& projectId);
    
    // Conversion helpers
    ProjectInfo projectToProjectInfo(const Project& project) const;
    std::unique_ptr<Project> loadProjectFromDatabase(const std::string& projectId);
    bool loadSceneObjects(Project& project);
    
    // Utility methods
    std::string formatTimestamp(const std::chrono::system_clock::time_point& timePoint) const;
    std::chrono::system_clock::time_point parseTimestamp(const std::string& timestamp) const;
    bool updateProjectObjectCount(const std::string& projectId);
    
    // Auto-save helpers
    bool updateAutoSaveConfig(const std::string& projectId, bool enabled, int intervalSeconds);
    bool loadAutoSaveStatus();
};

} // namespace Persistence
} // namespace KitchenCAD