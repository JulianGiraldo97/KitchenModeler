#include "ProjectController.h"
#include "../utils/Logger.h"
#include <algorithm>
#include <regex>
#include <set>

namespace KitchenCAD {
namespace Controllers {

ProjectController::ProjectController(std::unique_ptr<ProjectManager> projectManager)
    : projectManager_(std::move(projectManager))
{
    setupCallbacks();
}

ProjectOperationResult ProjectController::createProject(const std::string& name, 
                                                       const RoomDimensions& dimensions,
                                                       const std::string& description) {
    if (!validateProjectName(name)) {
        return ProjectOperationResult(false, "Invalid project name: " + name);
    }
    
    if (!validateRoomDimensions(dimensions)) {
        return ProjectOperationResult(false, "Invalid room dimensions");
    }
    
    try {
        ProjectMetadata metadata = createProjectMetadata(name, dimensions, description);
        std::string projectId = projectManager_->createProject(metadata);
        
        if (projectId.empty()) {
            return ProjectOperationResult(false, "Failed to create project");
        }
        
        notifyProjectOpened(projectId);
        return ProjectOperationResult(true, "Project created successfully", projectId);
        
    } catch (const std::exception& e) {
        std::string error = "Failed to create project: " + std::string(e.what());
        notifyError(error);
        return ProjectOperationResult(false, error);
    }
}

ProjectOperationResult ProjectController::createProjectFromTemplate(const std::string& templateId, 
                                                                   const std::string& projectName) {
    if (!validateProjectName(projectName)) {
        return ProjectOperationResult(false, "Invalid project name: " + projectName);
    }
    
    try {
        std::string projectId = projectManager_->createProjectFromTemplate(templateId, projectName);
        
        if (projectId.empty()) {
            return ProjectOperationResult(false, "Failed to create project from template");
        }
        
        notifyProjectOpened(projectId);
        return ProjectOperationResult(true, "Project created from template successfully", projectId);
        
    } catch (const std::exception& e) {
        std::string error = "Failed to create project from template: " + std::string(e.what());
        notifyError(error);
        return ProjectOperationResult(false, error);
    }
}

ProjectOperationResult ProjectController::openProject(const std::string& projectId) {
    if (projectId.empty()) {
        return ProjectOperationResult(false, "Invalid project ID");
    }
    
    try {
        // Check if we need to save current project first
        if (hasOpenProject() && hasUnsavedChanges()) {
            auto saveResult = saveProject();
            if (!saveResult.success) {
                return ProjectOperationResult(false, "Failed to save current project before opening new one");
            }
        }
        
        bool success = projectManager_->loadProject(projectId);
        
        if (!success) {
            return ProjectOperationResult(false, "Failed to open project: " + projectId);
        }
        
        notifyProjectOpened(projectId);
        return ProjectOperationResult(true, "Project opened successfully", projectId);
        
    } catch (const std::exception& e) {
        std::string error = "Failed to open project: " + std::string(e.what());
        notifyError(error);
        return ProjectOperationResult(false, error);
    }
}

ProjectOperationResult ProjectController::saveProject() {
    if (!hasOpenProject()) {
        return ProjectOperationResult(false, "No project is currently open");
    }
    
    try {
        bool success = projectManager_->saveProject();
        
        if (!success) {
            return ProjectOperationResult(false, "Failed to save project");
        }
        
        std::string projectId = getCurrentProjectId();
        notifyProjectSaved(projectId);
        return ProjectOperationResult(true, "Project saved successfully", projectId);
        
    } catch (const std::exception& e) {
        std::string error = "Failed to save project: " + std::string(e.what());
        notifyError(error);
        return ProjectOperationResult(false, error);
    }
}

ProjectOperationResult ProjectController::saveProjectAs(const std::string& newName) {
    if (!hasOpenProject()) {
        return ProjectOperationResult(false, "No project is currently open");
    }
    
    if (!validateProjectName(newName)) {
        return ProjectOperationResult(false, "Invalid project name: " + newName);
    }
    
    try {
        std::string newProjectId = projectManager_->saveProjectAs(newName);
        
        if (newProjectId.empty()) {
            return ProjectOperationResult(false, "Failed to save project as: " + newName);
        }
        
        notifyProjectSaved(newProjectId);
        return ProjectOperationResult(true, "Project saved as: " + newName, newProjectId);
        
    } catch (const std::exception& e) {
        std::string error = "Failed to save project as: " + std::string(e.what());
        notifyError(error);
        return ProjectOperationResult(false, error);
    }
}

ProjectOperationResult ProjectController::closeProject(bool forceSave) {
    if (!hasOpenProject()) {
        return ProjectOperationResult(true, "No project is currently open");
    }
    
    try {
        std::string projectId = getCurrentProjectId();
        
        // Save if needed
        if (hasUnsavedChanges()) {
            if (forceSave) {
                auto saveResult = saveProject();
                if (!saveResult.success) {
                    return ProjectOperationResult(false, "Failed to save project before closing");
                }
            } else {
                return ProjectOperationResult(false, "Project has unsaved changes");
            }
        }
        
        bool success = projectManager_->closeProject(forceSave);
        
        if (!success) {
            return ProjectOperationResult(false, "Failed to close project");
        }
        
        notifyProjectClosed(projectId);
        return ProjectOperationResult(true, "Project closed successfully", projectId);
        
    } catch (const std::exception& e) {
        std::string error = "Failed to close project: " + std::string(e.what());
        notifyError(error);
        return ProjectOperationResult(false, error);
    }
}

ProjectOperationResult ProjectController::deleteProject(const std::string& projectId) {
    if (projectId.empty()) {
        return ProjectOperationResult(false, "Invalid project ID");
    }
    
    // Don't allow deleting currently open project
    if (hasOpenProject() && getCurrentProjectId() == projectId) {
        return ProjectOperationResult(false, "Cannot delete currently open project");
    }
    
    try {
        bool success = projectManager_->deleteProject(projectId);
        
        if (!success) {
            return ProjectOperationResult(false, "Failed to delete project: " + projectId);
        }
        
        return ProjectOperationResult(true, "Project deleted successfully", projectId);
        
    } catch (const std::exception& e) {
        std::string error = "Failed to delete project: " + std::string(e.what());
        notifyError(error);
        return ProjectOperationResult(false, error);
    }
}

Project* ProjectController::getCurrentProject() const {
    return projectManager_->getCurrentProject();
}

bool ProjectController::hasOpenProject() const {
    return projectManager_->hasCurrentProject();
}

std::string ProjectController::getCurrentProjectId() const {
    return projectManager_->getCurrentProjectId();
}

bool ProjectController::hasUnsavedChanges() const {
    return projectManager_->hasUnsavedChanges();
}

std::vector<ProjectInfo> ProjectController::listProjects() const {
    try {
        return projectManager_->listProjects();
    } catch (const std::exception& e) {
        notifyError("Failed to list projects: " + std::string(e.what()));
        return {};
    }
}

std::vector<ProjectInfo> ProjectController::getRecentProjects(size_t maxCount) const {
    try {
        return projectManager_->listRecentProjects(maxCount);
    } catch (const std::exception& e) {
        notifyError("Failed to get recent projects: " + std::string(e.what()));
        return {};
    }
}

std::vector<ProjectInfo> ProjectController::searchProjects(const std::string& searchTerm) const {
    try {
        return projectManager_->searchProjects(searchTerm);
    } catch (const std::exception& e) {
        notifyError("Failed to search projects: " + std::string(e.what()));
        return {};
    }
}

std::optional<ProjectInfo> ProjectController::getProjectInfo(const std::string& projectId) const {
    try {
        return projectManager_->getProjectInfo(projectId);
    } catch (const std::exception& e) {
        notifyError("Failed to get project info: " + std::string(e.what()));
        return std::nullopt;
    }
}

bool ProjectController::updateProjectMetadata(const std::string& name, const std::string& description) {
    if (!hasOpenProject()) {
        notifyError("No project is currently open");
        return false;
    }
    
    if (!validateProjectName(name)) {
        notifyError("Invalid project name: " + name);
        return false;
    }
    
    try {
        Project* project = getCurrentProject();
        if (project) {
            project->setName(name);
            project->setDescription(description);
            markProjectAsModified();
            return true;
        }
        
        return false;
        
    } catch (const std::exception& e) {
        notifyError("Failed to update project metadata: " + std::string(e.what()));
        return false;
    }
}

bool ProjectController::updateRoomDimensions(const RoomDimensions& dimensions) {
    if (!hasOpenProject()) {
        notifyError("No project is currently open");
        return false;
    }
    
    if (!validateRoomDimensions(dimensions)) {
        notifyError("Invalid room dimensions");
        return false;
    }
    
    try {
        Project* project = getCurrentProject();
        if (project) {
            project->setDimensions(dimensions);
            markProjectAsModified();
            return true;
        }
        
        return false;
        
    } catch (const std::exception& e) {
        notifyError("Failed to update room dimensions: " + std::string(e.what()));
        return false;
    }
}

void ProjectController::markProjectAsModified() {
    if (projectManager_) {
        projectManager_->markAsModified();
        notifyProjectChanged("Project modified");
    }
}

void ProjectController::configureAutoSave(bool enabled, int intervalSeconds, int maxBackups) {
    if (!projectManager_) {
        return;
    }
    
    ProjectManager::AutoSaveConfig config;
    config.enabled = enabled;
    config.intervalSeconds = intervalSeconds;
    config.maxBackups = maxBackups;
    
    projectManager_->setAutoSaveConfig(config);
}

ProjectManager::AutoSaveConfig ProjectController::getAutoSaveConfig() const {
    if (projectManager_) {
        return projectManager_->getAutoSaveConfig();
    }
    
    return ProjectManager::AutoSaveConfig();
}

void ProjectController::setAutoSaveEnabled(bool enabled) {
    if (projectManager_) {
        projectManager_->setAutoSaveEnabled(enabled);
    }
}

bool ProjectController::shouldAutoSave() const {
    return projectManager_ && projectManager_->shouldAutoSave();
}

bool ProjectController::performAutoSave() {
    if (!projectManager_) {
        return false;
    }
    
    try {
        bool success = projectManager_->performAutoSave();
        if (success) {
            notifyAutoSave("Auto-save completed");
        }
        return success;
        
    } catch (const std::exception& e) {
        notifyError("Auto-save failed: " + std::string(e.what()));
        return false;
    }
}

bool ProjectController::forceAutoSave() {
    if (!projectManager_) {
        return false;
    }
    
    try {
        bool success = projectManager_->forceAutoSave();
        if (success) {
            notifyAutoSave("Force auto-save completed");
        }
        return success;
        
    } catch (const std::exception& e) {
        notifyError("Force auto-save failed: " + std::string(e.what()));
        return false;
    }
}

std::vector<ProjectManager::ProjectTemplate> ProjectController::getAvailableTemplates() const {
    if (projectManager_) {
        return projectManager_->getTemplates();
    }
    
    return {};
}

ProjectOperationResult ProjectController::createTemplateFromCurrentProject(const std::string& templateName, 
                                                                          const std::string& description) {
    if (!hasOpenProject()) {
        return ProjectOperationResult(false, "No project is currently open");
    }
    
    if (templateName.empty()) {
        return ProjectOperationResult(false, "Template name cannot be empty");
    }
    
    try {
        std::string templateId = projectManager_->createTemplateFromProject(templateName, description);
        
        if (templateId.empty()) {
            return ProjectOperationResult(false, "Failed to create template");
        }
        
        return ProjectOperationResult(true, "Template created successfully", templateId);
        
    } catch (const std::exception& e) {
        std::string error = "Failed to create template: " + std::string(e.what());
        notifyError(error);
        return ProjectOperationResult(false, error);
    }
}

bool ProjectController::addTemplate(const ProjectManager::ProjectTemplate& projectTemplate) {
    if (projectManager_) {
        projectManager_->addTemplate(projectTemplate);
        return true;
    }
    
    return false;
}

bool ProjectController::removeTemplate(const std::string& templateId) {
    if (projectManager_) {
        return projectManager_->removeTemplate(templateId);
    }
    
    return false;
}

ProjectOperationResult ProjectController::duplicateProject(const std::string& projectId, const std::string& newName) {
    if (projectId.empty()) {
        return ProjectOperationResult(false, "Invalid project ID");
    }
    
    if (!validateProjectName(newName)) {
        return ProjectOperationResult(false, "Invalid project name: " + newName);
    }
    
    try {
        std::string newProjectId = projectManager_->duplicateProject(projectId, newName);
        
        if (newProjectId.empty()) {
            return ProjectOperationResult(false, "Failed to duplicate project");
        }
        
        return ProjectOperationResult(true, "Project duplicated successfully", newProjectId);
        
    } catch (const std::exception& e) {
        std::string error = "Failed to duplicate project: " + std::string(e.what());
        notifyError(error);
        return ProjectOperationResult(false, error);
    }
}

ProjectOperationResult ProjectController::exportProject(const std::string& projectId, const std::string& filePath) {
    if (projectId.empty()) {
        return ProjectOperationResult(false, "Invalid project ID");
    }
    
    if (filePath.empty()) {
        return ProjectOperationResult(false, "Invalid file path");
    }
    
    try {
        bool success = projectManager_->exportProject(projectId, filePath);
        
        if (!success) {
            return ProjectOperationResult(false, "Failed to export project");
        }
        
        return ProjectOperationResult(true, "Project exported successfully", projectId);
        
    } catch (const std::exception& e) {
        std::string error = "Failed to export project: " + std::string(e.what());
        notifyError(error);
        return ProjectOperationResult(false, error);
    }
}

ProjectOperationResult ProjectController::importProject(const std::string& filePath) {
    if (filePath.empty()) {
        return ProjectOperationResult(false, "Invalid file path");
    }
    
    try {
        auto projectId = projectManager_->importProject(filePath);
        
        if (!projectId.has_value()) {
            return ProjectOperationResult(false, "Failed to import project");
        }
        
        return ProjectOperationResult(true, "Project imported successfully", projectId.value());
        
    } catch (const std::exception& e) {
        std::string error = "Failed to import project: " + std::string(e.what());
        notifyError(error);
        return ProjectOperationResult(false, error);
    }
}

ProjectOperationResult ProjectController::createBackup(const std::string& backupName) {
    if (!hasOpenProject()) {
        return ProjectOperationResult(false, "No project is currently open");
    }
    
    try {
        bool success = projectManager_->createBackup(backupName);
        
        if (!success) {
            return ProjectOperationResult(false, "Failed to create backup");
        }
        
        return ProjectOperationResult(true, "Backup created successfully");
        
    } catch (const std::exception& e) {
        std::string error = "Failed to create backup: " + std::string(e.what());
        notifyError(error);
        return ProjectOperationResult(false, error);
    }
}

ProjectOperationResult ProjectController::restoreFromBackup(const std::string& backupPath) {
    if (backupPath.empty()) {
        return ProjectOperationResult(false, "Invalid backup path");
    }
    
    try {
        bool success = projectManager_->restoreFromBackup(backupPath);
        
        if (!success) {
            return ProjectOperationResult(false, "Failed to restore from backup");
        }
        
        return ProjectOperationResult(true, "Project restored from backup successfully");
        
    } catch (const std::exception& e) {
        std::string error = "Failed to restore from backup: " + std::string(e.what());
        notifyError(error);
        return ProjectOperationResult(false, error);
    }
}

std::vector<std::string> ProjectController::validateCurrentProject() const {
    if (!hasOpenProject()) {
        return {"No project is currently open"};
    }
    
    try {
        Project* project = getCurrentProject();
        if (project) {
            return project->validate();
        }
        
        return {"Failed to access current project"};
        
    } catch (const std::exception& e) {
        return {"Failed to validate project: " + std::string(e.what())};
    }
}

ProjectManager::ProjectStatistics ProjectController::getProjectStatistics() const {
    if (projectManager_) {
        return projectManager_->getStatistics();
    }
    
    return ProjectManager::ProjectStatistics();
}

void ProjectController::cleanupProjectFiles() {
    if (projectManager_) {
        projectManager_->cleanup();
    }
}

bool ProjectController::validateRepository() {
    if (projectManager_) {
        return projectManager_->validateRepository();
    }
    
    return false;
}

std::chrono::seconds ProjectController::getTimeSinceLastSave() const {
    if (projectManager_) {
        return projectManager_->getTimeSinceLastSave();
    }
    
    return std::chrono::seconds(0);
}

std::chrono::seconds ProjectController::getTimeSinceLastAutoSave() const {
    if (projectManager_) {
        return projectManager_->getTimeSinceLastAutoSave();
    }
    
    return std::chrono::seconds(0);
}

bool ProjectController::setProjectSetting(const std::string& key, const std::string& value) {
    if (!hasOpenProject()) {
        return false;
    }
    
    // Implementation would store project-specific settings
    // This is a placeholder implementation
    return true;
}

std::string ProjectController::getProjectSetting(const std::string& key, const std::string& defaultValue) const {
    if (!hasOpenProject()) {
        return defaultValue;
    }
    
    // Implementation would retrieve project-specific settings
    // This is a placeholder implementation
    return defaultValue;
}

bool ProjectController::removeProjectSetting(const std::string& key) {
    if (!hasOpenProject()) {
        return false;
    }
    
    // Implementation would remove project-specific settings
    // This is a placeholder implementation
    return true;
}

bool ProjectController::isValidProjectName(const std::string& name) {
    if (name.empty() || name.length() > 255) {
        return false;
    }
    
    // Check for invalid characters
    std::regex invalidChars(R"([<>:"/\\|?*])");
    if (std::regex_search(name, invalidChars)) {
        return false;
    }
    
    // Check for reserved names
    std::vector<std::string> reservedNames = {"CON", "PRN", "AUX", "NUL", "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9", "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"};
    std::string upperName = name;
    std::transform(upperName.begin(), upperName.end(), upperName.begin(), ::toupper);
    
    for (const auto& reserved : reservedNames) {
        if (upperName == reserved) {
            return false;
        }
    }
    
    return true;
}

std::string ProjectController::sanitizeProjectName(const std::string& name) {
    std::string sanitized = name;
    
    // Replace invalid characters with underscores
    std::regex invalidChars(R"([<>:"/\\|?*])");
    sanitized = std::regex_replace(sanitized, invalidChars, "_");
    
    // Trim whitespace
    sanitized.erase(0, sanitized.find_first_not_of(" \t\n\r\f\v"));
    sanitized.erase(sanitized.find_last_not_of(" \t\n\r\f\v") + 1);
    
    // Ensure not empty
    if (sanitized.empty()) {
        sanitized = "Untitled";
    }
    
    // Truncate if too long
    if (sanitized.length() > 255) {
        sanitized = sanitized.substr(0, 255);
    }
    
    return sanitized;
}

std::string ProjectController::generateUniqueProjectName(const std::string& baseName) const {
    std::string sanitizedBase = sanitizeProjectName(baseName);
    std::string uniqueName = sanitizedBase;
    
    auto projects = listProjects();
    std::set<std::string> existingNames;
    for (const auto& project : projects) {
        existingNames.insert(project.name);
    }
    
    int counter = 1;
    while (existingNames.find(uniqueName) != existingNames.end()) {
        uniqueName = sanitizedBase + " (" + std::to_string(counter) + ")";
        counter++;
    }
    
    return uniqueName;
}

void ProjectController::setupCallbacks() {
    if (!projectManager_) {
        return;
    }
    
    projectManager_->setProjectChangedCallback([this](const std::string& message) {
        notifyProjectChanged(message);
    });
    
    projectManager_->setAutoSaveCallback([this](const std::string& message) {
        notifyAutoSave(message);
    });
    
    projectManager_->setErrorCallback([this](const std::string& error) {
        notifyError(error);
    });
}

void ProjectController::notifyProjectOpened(const std::string& projectId) {
    if (projectOpenedCallback_) {
        projectOpenedCallback_(projectId);
    }
}

void ProjectController::notifyProjectClosed(const std::string& projectId) {
    if (projectClosedCallback_) {
        projectClosedCallback_(projectId);
    }
}

void ProjectController::notifyProjectSaved(const std::string& projectId) {
    if (projectSavedCallback_) {
        projectSavedCallback_(projectId);
    }
}

void ProjectController::notifyProjectChanged(const std::string& message) {
    if (projectChangedCallback_) {
        projectChangedCallback_(message);
    }
}

void ProjectController::notifyAutoSave(const std::string& message) {
    if (autoSaveCallback_) {
        autoSaveCallback_(message);
    }
}

void ProjectController::notifyError(const std::string& error) {
    if (errorCallback_) {
        errorCallback_(error);
    }
}

bool ProjectController::validateProjectName(const std::string& name) const {
    return isValidProjectName(name);
}

bool ProjectController::validateRoomDimensions(const RoomDimensions& dimensions) const {
    return dimensions.isValid() && 
           dimensions.width >= 0.1 && dimensions.width <= 100.0 &&  // 10cm to 100m
           dimensions.height >= 0.1 && dimensions.height <= 100.0 &&
           dimensions.depth >= 0.1 && dimensions.depth <= 100.0;
}

ProjectMetadata ProjectController::createProjectMetadata(const std::string& name, 
                                                        const RoomDimensions& dimensions,
                                                        const std::string& description) const {
    ProjectMetadata metadata;
    metadata.name = name;
    metadata.description = description;
    metadata.roomWidth = dimensions.width;
    metadata.roomHeight = dimensions.height;
    metadata.roomDepth = dimensions.depth;
    metadata.createdAt = std::chrono::system_clock::now();
    metadata.updatedAt = metadata.createdAt;
    
    return metadata;
}

} // namespace Controllers
} // namespace KitchenCAD