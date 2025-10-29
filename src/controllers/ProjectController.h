#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include "../services/ProjectManager.h"
#include "../models/Project.h"
#include "../interfaces/IProjectRepository.h"
#include <chrono>
#include <unordered_map>

namespace KitchenCAD {
namespace Controllers {

using namespace Models;
using namespace Services;

/**
 * @brief Project operation result
 */
struct ProjectOperationResult {
    bool success = false;
    std::string message;
    std::string projectId;
    
    ProjectOperationResult() = default;
    ProjectOperationResult(bool success, const std::string& message, const std::string& id = "")
        : success(success), message(message), projectId(id) {}
};

/**
 * @brief Project controller for managing project operations
 * 
 * This controller handles all project-related operations including creation,
 * loading, saving, and project lifecycle management. Implements requirements 1.1, 10.1, 10.2, 10.3, 10.4, 10.5.
 */
class ProjectController {
private:
    std::unique_ptr<ProjectManager> projectManager_;
    
    // Callbacks
    std::function<void(const std::string&)> projectOpenedCallback_;
    std::function<void(const std::string&)> projectClosedCallback_;
    std::function<void(const std::string&)> projectSavedCallback_;
    std::function<void(const std::string&)> projectChangedCallback_;
    std::function<void(const std::string&)> autoSaveCallback_;
    std::function<void(const std::string&)> errorCallback_;

public:
    /**
     * @brief Constructor
     */
    explicit ProjectController(std::unique_ptr<ProjectManager> projectManager);
    
    /**
     * @brief Destructor
     */
    ~ProjectController() = default;
    
    // Project lifecycle operations
    
    /**
     * @brief Create a new project
     */
    ProjectOperationResult createProject(const std::string& name, 
                                       const RoomDimensions& dimensions,
                                       const std::string& description = "");
    
    /**
     * @brief Create project from template
     */
    ProjectOperationResult createProjectFromTemplate(const std::string& templateId, 
                                                    const std::string& projectName);
    
    /**
     * @brief Open an existing project
     */
    ProjectOperationResult openProject(const std::string& projectId);
    
    /**
     * @brief Save current project
     */
    ProjectOperationResult saveProject();
    
    /**
     * @brief Save project with new name (Save As)
     */
    ProjectOperationResult saveProjectAs(const std::string& newName);
    
    /**
     * @brief Close current project
     */
    ProjectOperationResult closeProject(bool forceSave = false);
    
    /**
     * @brief Delete a project
     */
    ProjectOperationResult deleteProject(const std::string& projectId);
    
    // Project access and queries
    
    /**
     * @brief Get current project
     */
    Project* getCurrentProject() const;
    
    /**
     * @brief Check if a project is currently open
     */
    bool hasOpenProject() const;
    
    /**
     * @brief Get current project ID
     */
    std::string getCurrentProjectId() const;
    
    /**
     * @brief Check if current project has unsaved changes
     */
    bool hasUnsavedChanges() const;
    
    /**
     * @brief Get list of all projects
     */
    std::vector<ProjectInfo> listProjects() const;
    
    /**
     * @brief Get list of recent projects
     */
    std::vector<ProjectInfo> getRecentProjects(size_t maxCount = 10) const;
    
    /**
     * @brief Search projects by name or description
     */
    std::vector<ProjectInfo> searchProjects(const std::string& searchTerm) const;
    
    /**
     * @brief Get project information without opening
     */
    std::optional<ProjectInfo> getProjectInfo(const std::string& projectId) const;
    
    // Project modification
    
    /**
     * @brief Update project metadata
     */
    bool updateProjectMetadata(const std::string& name, const std::string& description);
    
    /**
     * @brief Update room dimensions
     */
    bool updateRoomDimensions(const RoomDimensions& dimensions);
    
    /**
     * @brief Mark current project as modified
     */
    void markProjectAsModified();
    
    // Auto-save management
    
    /**
     * @brief Configure auto-save settings
     */
    void configureAutoSave(bool enabled, int intervalSeconds = 30, int maxBackups = 5);
    
    /**
     * @brief Get auto-save configuration
     */
    ProjectManager::AutoSaveConfig getAutoSaveConfig() const;
    
    /**
     * @brief Enable/disable auto-save
     */
    void setAutoSaveEnabled(bool enabled);
    
    /**
     * @brief Check if auto-save should run
     */
    bool shouldAutoSave() const;
    
    /**
     * @brief Perform auto-save if needed
     */
    bool performAutoSave();
    
    /**
     * @brief Force auto-save now
     */
    bool forceAutoSave();
    
    // Template management
    
    /**
     * @brief Get available project templates
     */
    std::vector<ProjectManager::ProjectTemplate> getAvailableTemplates() const;
    
    /**
     * @brief Create template from current project
     */
    ProjectOperationResult createTemplateFromCurrentProject(const std::string& templateName, 
                                                           const std::string& description);
    
    /**
     * @brief Add a project template
     */
    bool addTemplate(const ProjectManager::ProjectTemplate& projectTemplate);
    
    /**
     * @brief Remove a project template
     */
    bool removeTemplate(const std::string& templateId);
    
    // Project operations
    
    /**
     * @brief Duplicate a project
     */
    ProjectOperationResult duplicateProject(const std::string& projectId, const std::string& newName);
    
    /**
     * @brief Export project to file
     */
    ProjectOperationResult exportProject(const std::string& projectId, const std::string& filePath);
    
    /**
     * @brief Import project from file
     */
    ProjectOperationResult importProject(const std::string& filePath);
    
    /**
     * @brief Create backup of current project
     */
    ProjectOperationResult createBackup(const std::string& backupName = "");
    
    /**
     * @brief Restore project from backup
     */
    ProjectOperationResult restoreFromBackup(const std::string& backupPath);
    
    // Project validation and maintenance
    
    /**
     * @brief Validate current project
     */
    std::vector<std::string> validateCurrentProject() const;
    
    /**
     * @brief Get project statistics
     */
    ProjectManager::ProjectStatistics getProjectStatistics() const;
    
    /**
     * @brief Cleanup old backups and temporary files
     */
    void cleanupProjectFiles();
    
    /**
     * @brief Validate repository integrity
     */
    bool validateRepository();
    
    // Time tracking
    
    /**
     * @brief Get time since last save
     */
    std::chrono::seconds getTimeSinceLastSave() const;
    
    /**
     * @brief Get time since last auto-save
     */
    std::chrono::seconds getTimeSinceLastAutoSave() const;
    
    // Project preferences and settings
    
    /**
     * @brief Set project-specific settings
     */
    bool setProjectSetting(const std::string& key, const std::string& value);
    
    /**
     * @brief Get project-specific setting
     */
    std::string getProjectSetting(const std::string& key, const std::string& defaultValue = "") const;
    
    /**
     * @brief Remove project setting
     */
    bool removeProjectSetting(const std::string& key);
    
    // Callbacks
    
    /**
     * @brief Set callback for project opened events
     */
    void setProjectOpenedCallback(std::function<void(const std::string&)> callback) {
        projectOpenedCallback_ = callback;
    }
    
    /**
     * @brief Set callback for project closed events
     */
    void setProjectClosedCallback(std::function<void(const std::string&)> callback) {
        projectClosedCallback_ = callback;
    }
    
    /**
     * @brief Set callback for project saved events
     */
    void setProjectSavedCallback(std::function<void(const std::string&)> callback) {
        projectSavedCallback_ = callback;
    }
    
    /**
     * @brief Set callback for project changed events
     */
    void setProjectChangedCallback(std::function<void(const std::string&)> callback) {
        projectChangedCallback_ = callback;
    }
    
    /**
     * @brief Set callback for auto-save events
     */
    void setAutoSaveCallback(std::function<void(const std::string&)> callback) {
        autoSaveCallback_ = callback;
    }
    
    /**
     * @brief Set callback for errors
     */
    void setErrorCallback(std::function<void(const std::string&)> callback) {
        errorCallback_ = callback;
    }
    
    // Utility methods
    
    /**
     * @brief Check if project name is valid
     */
    static bool isValidProjectName(const std::string& name);
    
    /**
     * @brief Sanitize project name for file system
     */
    static std::string sanitizeProjectName(const std::string& name);
    
    /**
     * @brief Generate unique project name
     */
    std::string generateUniqueProjectName(const std::string& baseName) const;

private:
    /**
     * @brief Setup project manager callbacks
     */
    void setupCallbacks();
    
    /**
     * @brief Notify callbacks
     */
    void notifyProjectOpened(const std::string& projectId);
    void notifyProjectClosed(const std::string& projectId);
    void notifyProjectSaved(const std::string& projectId);
    void notifyProjectChanged(const std::string& message);
    void notifyAutoSave(const std::string& message);
    void notifyError(const std::string& error);
    
    /**
     * @brief Validate project operation parameters
     */
    bool validateProjectName(const std::string& name) const;
    bool validateRoomDimensions(const RoomDimensions& dimensions) const;
    
    /**
     * @brief Create project metadata from parameters
     */
    ProjectMetadata createProjectMetadata(const std::string& name, 
                                        const RoomDimensions& dimensions,
                                        const std::string& description) const;
};

} // namespace Controllers
} // namespace KitchenCAD