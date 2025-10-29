#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <functional>
#include <chrono>
#include <unordered_map>
#include "../models/Project.h"
#include "../interfaces/IProjectRepository.h"

namespace KitchenCAD {
namespace Services {

using namespace Models;

/**
 * @brief Project lifecycle management service
 * 
 * This service manages the complete lifecycle of projects including creation,
 * loading, saving, auto-save, templates, and project operations.
 * Implements requirements 1.1, 10.1, 10.2, 10.3, 10.4, 10.5
 */
class ProjectManager {
public:
    /**
     * @brief Auto-save configuration
     */
    struct AutoSaveConfig {
        bool enabled = true;
        int intervalSeconds = 30;
        int maxBackups = 5;
        std::string backupDirectory = "backups";
    };
    
    /**
     * @brief Project template information
     */
    struct ProjectTemplate {
        std::string id;
        std::string name;
        std::string description;
        RoomDimensions defaultDimensions;
        std::vector<std::string> preloadedObjects;
        std::string thumbnailPath;
    };

private:
    std::unique_ptr<IProjectRepository> repository_;
    std::unique_ptr<Project> currentProject_;
    AutoSaveConfig autoSaveConfig_;
    std::vector<ProjectTemplate> templates_;
    
    // Auto-save management
    std::chrono::system_clock::time_point lastSave_;
    std::chrono::system_clock::time_point lastAutoSave_;
    bool hasUnsavedChanges_;
    
    // Callbacks
    std::function<void(const std::string&)> projectChangedCallback_;
    std::function<void(const std::string&)> autoSaveCallback_;
    std::function<void(const std::string&)> errorCallback_;

public:
    /**
     * @brief Constructor
     */
    explicit ProjectManager(std::unique_ptr<IProjectRepository> repository);
    
    /**
     * @brief Destructor - ensures current project is saved
     */
    ~ProjectManager();
    
    // Project lifecycle operations
    
    /**
     * @brief Create a new project
     */
    std::string createProject(const ProjectMetadata& metadata);
    
    /**
     * @brief Create project from template
     */
    std::string createProjectFromTemplate(const std::string& templateId, 
                                         const std::string& projectName);
    
    /**
     * @brief Load an existing project
     */
    bool loadProject(const std::string& projectId);
    
    /**
     * @brief Save current project
     */
    bool saveProject();
    
    /**
     * @brief Save project with new name (Save As)
     */
    std::string saveProjectAs(const std::string& newName);
    
    /**
     * @brief Close current project
     */
    bool closeProject(bool forceSave = false);
    
    /**
     * @brief Delete a project
     */
    bool deleteProject(const std::string& projectId);
    
    // Project access
    
    /**
     * @brief Get current project
     */
    Project* getCurrentProject() const { return currentProject_.get(); }
    
    /**
     * @brief Check if a project is currently loaded
     */
    bool hasCurrentProject() const { return currentProject_ != nullptr; }
    
    /**
     * @brief Get current project ID
     */
    std::string getCurrentProjectId() const;
    
    /**
     * @brief Check if current project has unsaved changes
     */
    bool hasUnsavedChanges() const { return hasUnsavedChanges_; }
    
    // Project queries
    
    /**
     * @brief List all projects
     */
    std::vector<ProjectInfo> listProjects() const;
    
    /**
     * @brief List recent projects
     */
    std::vector<ProjectInfo> listRecentProjects(size_t maxCount = 10) const;
    
    /**
     * @brief Search projects by name or description
     */
    std::vector<ProjectInfo> searchProjects(const std::string& searchTerm) const;
    
    /**
     * @brief Get project information without loading
     */
    std::optional<ProjectInfo> getProjectInfo(const std::string& projectId) const;
    
    // Auto-save management
    
    /**
     * @brief Configure auto-save settings
     */
    void setAutoSaveConfig(const AutoSaveConfig& config);
    
    /**
     * @brief Get current auto-save configuration
     */
    const AutoSaveConfig& getAutoSaveConfig() const { return autoSaveConfig_; }
    
    /**
     * @brief Enable/disable auto-save for current project
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
    const std::vector<ProjectTemplate>& getTemplates() const { return templates_; }
    
    /**
     * @brief Add a project template
     */
    void addTemplate(const ProjectTemplate& projectTemplate);
    
    /**
     * @brief Remove a project template
     */
    bool removeTemplate(const std::string& templateId);
    
    /**
     * @brief Create template from current project
     */
    std::string createTemplateFromProject(const std::string& templateName, 
                                         const std::string& description);
    
    // Project operations
    
    /**
     * @brief Duplicate a project
     */
    std::string duplicateProject(const std::string& projectId, const std::string& newName);
    
    /**
     * @brief Export project to file
     */
    bool exportProject(const std::string& projectId, const std::string& filePath);
    
    /**
     * @brief Import project from file
     */
    std::optional<std::string> importProject(const std::string& filePath);
    
    /**
     * @brief Create backup of current project
     */
    bool createBackup(const std::string& backupName = "");
    
    /**
     * @brief Restore project from backup
     */
    bool restoreFromBackup(const std::string& backupPath);
    
    // Change tracking
    
    /**
     * @brief Mark current project as modified
     */
    void markAsModified();
    
    /**
     * @brief Mark current project as saved
     */
    void markAsSaved();
    
    /**
     * @brief Get time since last save
     */
    std::chrono::seconds getTimeSinceLastSave() const;
    
    /**
     * @brief Get time since last auto-save
     */
    std::chrono::seconds getTimeSinceLastAutoSave() const;
    
    // Callbacks
    
    /**
     * @brief Set callback for project changes
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
    
    // Statistics and maintenance
    
    /**
     * @brief Get project statistics
     */
    struct ProjectStatistics {
        size_t totalProjects;
        size_t totalObjects;
        double totalValue;
        std::string mostUsedCategory;
        std::chrono::system_clock::time_point lastActivity;
    };
    
    ProjectStatistics getStatistics() const;
    
    /**
     * @brief Cleanup old backups and temporary files
     */
    void cleanup();
    
    /**
     * @brief Validate repository integrity
     */
    bool validateRepository();

private:
    /**
     * @brief Initialize default templates
     */
    void initializeDefaultTemplates();
    
    /**
     * @brief Notify project changed callback
     */
    void notifyProjectChanged(const std::string& message);
    
    /**
     * @brief Notify auto-save callback
     */
    void notifyAutoSave(const std::string& message);
    
    /**
     * @brief Notify error callback
     */
    void notifyError(const std::string& error) const;
    
    /**
     * @brief Update timestamps
     */
    void updateTimestamps();
    
    /**
     * @brief Generate backup filename
     */
    std::string generateBackupFilename(const std::string& projectId, 
                                      const std::string& suffix = "") const;
};

} // namespace Services
} // namespace KitchenCAD