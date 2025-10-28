#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>

namespace KitchenCAD {

// Forward declarations
namespace Models {
    class Project;
}
using Project = Models::Project;

/**
 * @brief Metadata information about a project
 */
struct ProjectInfo {
    std::string id;
    std::string name;
    std::string description;
    std::string createdAt;
    std::string updatedAt;
    double roomWidth;
    double roomHeight;
    double roomDepth;
    size_t objectCount;
    std::string thumbnailPath;
    
    ProjectInfo() = default;
    ProjectInfo(const std::string& id, const std::string& name)
        : id(id), name(name), roomWidth(0), roomHeight(0), roomDepth(0), objectCount(0) {}
};

/**
 * @brief Metadata for creating new projects
 */
struct ProjectMetadata {
    std::string name;
    std::string description;
    double roomWidth;
    double roomHeight;
    double roomDepth;
    std::string templateId;  // Optional template to base project on
    
    ProjectMetadata() = default;
    ProjectMetadata(const std::string& name, double width, double height, double depth)
        : name(name), roomWidth(width), roomHeight(height), roomDepth(depth) {}
};

/**
 * @brief Interface for project persistence operations
 * 
 * This interface defines the contract for project storage and retrieval,
 * typically implemented using SQLite for local storage.
 */
class IProjectRepository {
public:
    virtual ~IProjectRepository() = default;
    
    // Project CRUD operations
    virtual std::string createProject(const ProjectMetadata& metadata) = 0;
    virtual std::optional<std::unique_ptr<Project>> loadProject(const std::string& projectId) = 0;
    virtual bool saveProject(const Project& project) = 0;
    virtual bool deleteProject(const std::string& projectId) = 0;
    
    // Project queries
    virtual std::vector<ProjectInfo> listProjects() = 0;
    virtual std::vector<ProjectInfo> listRecentProjects(size_t maxCount = 10) = 0;
    virtual std::vector<ProjectInfo> searchProjects(const std::string& searchTerm) = 0;
    virtual std::optional<ProjectInfo> getProjectInfo(const std::string& projectId) = 0;
    
    // Project metadata operations
    virtual bool updateProjectMetadata(const std::string& projectId, const ProjectMetadata& metadata) = 0;
    virtual bool setProjectThumbnail(const std::string& projectId, const std::string& thumbnailPath) = 0;
    
    // Project validation
    virtual bool projectExists(const std::string& projectId) = 0;
    virtual bool isValidProjectName(const std::string& name) = 0;
    
    // Backup and restore
    virtual bool exportProject(const std::string& projectId, const std::string& filePath) = 0;
    virtual std::optional<std::string> importProject(const std::string& filePath) = 0;
    
    // Database maintenance
    virtual bool vacuum() = 0;  // Optimize database
    virtual bool backup(const std::string& backupPath) = 0;
    virtual bool restore(const std::string& backupPath) = 0;
    
    // Statistics
    virtual size_t getTotalProjectCount() = 0;
    virtual size_t getDatabaseSize() = 0;  // Size in bytes
    
    // Auto-save support
    virtual bool enableAutoSave(const std::string& projectId, int intervalSeconds = 30) = 0;
    virtual bool disableAutoSave(const std::string& projectId) = 0;
    virtual bool isAutoSaveEnabled(const std::string& projectId) = 0;
};

} // namespace KitchenCAD