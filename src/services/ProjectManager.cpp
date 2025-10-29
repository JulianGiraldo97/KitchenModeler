#include "ProjectManager.h"
#include "../utils/Logger.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>

namespace KitchenCAD {
namespace Services {

ProjectManager::ProjectManager(std::unique_ptr<IProjectRepository> repository)
    : repository_(std::move(repository))
    , hasUnsavedChanges_(false)
{
    initializeDefaultTemplates();
    updateTimestamps();
}

ProjectManager::~ProjectManager() {
    if (hasCurrentProject() && hasUnsavedChanges_) {
        // Attempt to save before destruction
        saveProject();
    }
}

std::string ProjectManager::createProject(const ProjectMetadata& metadata) {
    try {
        // Validate metadata
        if (metadata.name.empty()) {
            notifyError("Project name cannot be empty");
            return "";
        }
        
        if (!repository_->isValidProjectName(metadata.name)) {
            notifyError("Invalid project name: " + metadata.name);
            return "";
        }
        
        // Create project in repository
        std::string projectId = repository_->createProject(metadata);
        if (projectId.empty()) {
            notifyError("Failed to create project in repository");
            return "";
        }
        
        // Load the newly created project
        if (!loadProject(projectId)) {
            notifyError("Failed to load newly created project");
            repository_->deleteProject(projectId);
            return "";
        }
        
        notifyProjectChanged("Project created: " + metadata.name);
        return projectId;
        
    } catch (const std::exception& e) {
        notifyError("Exception creating project: " + std::string(e.what()));
        return "";
    }
}

std::string ProjectManager::createProjectFromTemplate(const std::string& templateId, 
                                                     const std::string& projectName) {
    // Find template
    auto templateIt = std::find_if(templates_.begin(), templates_.end(),
        [&templateId](const ProjectTemplate& t) { return t.id == templateId; });
    
    if (templateIt == templates_.end()) {
        notifyError("Template not found: " + templateId);
        return "";
    }
    
    // Create project metadata from template
    ProjectMetadata metadata;
    metadata.name = projectName;
    metadata.description = "Created from template: " + templateIt->name;
    metadata.roomWidth = templateIt->defaultDimensions.width;
    metadata.roomHeight = templateIt->defaultDimensions.height;
    metadata.roomDepth = templateIt->defaultDimensions.depth;
    metadata.templateId = templateId;
    
    std::string projectId = createProject(metadata);
    if (projectId.empty()) {
        return "";
    }
    
    // TODO: Add preloaded objects from template
    // This would require integration with CatalogService
    
    return projectId;
}

bool ProjectManager::loadProject(const std::string& projectId) {
    try {
        // Save current project if needed
        if (hasCurrentProject() && hasUnsavedChanges_) {
            if (!saveProject()) {
                notifyError("Failed to save current project before loading new one");
                return false;
            }
        }
        
        // Load project from repository
        auto projectOpt = repository_->loadProject(projectId);
        if (!projectOpt || !projectOpt.value()) {
            notifyError("Failed to load project: " + projectId);
            return false;
        }
        
        currentProject_ = std::move(projectOpt.value());
        hasUnsavedChanges_ = false;
        updateTimestamps();
        
        notifyProjectChanged("Project loaded: " + currentProject_->getName());
        return true;
        
    } catch (const std::exception& e) {
        notifyError("Exception loading project: " + std::string(e.what()));
        return false;
    }
}

bool ProjectManager::saveProject() {
    if (!hasCurrentProject()) {
        notifyError("No project to save");
        return false;
    }
    
    try {
        if (repository_->saveProject(*currentProject_)) {
            hasUnsavedChanges_ = false;
            lastSave_ = std::chrono::system_clock::now();
            notifyProjectChanged("Project saved: " + currentProject_->getName());
            return true;
        } else {
            notifyError("Failed to save project to repository");
            return false;
        }
        
    } catch (const std::exception& e) {
        notifyError("Exception saving project: " + std::string(e.what()));
        return false;
    }
}

std::string ProjectManager::saveProjectAs(const std::string& newName) {
    if (!hasCurrentProject()) {
        notifyError("No project to save");
        return "";
    }
    
    try {
        // Create new project metadata
        ProjectMetadata metadata;
        metadata.name = newName;
        metadata.description = currentProject_->getDescription();
        metadata.roomWidth = currentProject_->getDimensions().width;
        metadata.roomHeight = currentProject_->getDimensions().height;
        metadata.roomDepth = currentProject_->getDimensions().depth;
        
        // Create new project
        std::string newProjectId = repository_->createProject(metadata);
        if (newProjectId.empty()) {
            notifyError("Failed to create new project for Save As");
            return "";
        }
        
        // Update current project with new ID and name
        currentProject_->setId(newProjectId);
        currentProject_->setName(newName);
        
        // Save the project
        if (saveProject()) {
            notifyProjectChanged("Project saved as: " + newName);
            return newProjectId;
        } else {
            repository_->deleteProject(newProjectId);
            notifyError("Failed to save project with new name");
            return "";
        }
        
    } catch (const std::exception& e) {
        notifyError("Exception in Save As: " + std::string(e.what()));
        return "";
    }
}

bool ProjectManager::closeProject(bool forceSave) {
    if (!hasCurrentProject()) {
        return true;
    }
    
    if (hasUnsavedChanges_) {
        if (forceSave) {
            if (!saveProject()) {
                notifyError("Failed to save project before closing");
                return false;
            }
        } else {
            notifyProjectChanged("Project closed with unsaved changes");
        }
    }
    
    std::string projectName = currentProject_->getName();
    currentProject_.reset();
    hasUnsavedChanges_ = false;
    
    notifyProjectChanged("Project closed: " + projectName);
    return true;
}

bool ProjectManager::deleteProject(const std::string& projectId) {
    try {
        // Close project if it's currently loaded
        if (hasCurrentProject() && currentProject_->getId() == projectId) {
            closeProject(false); // Don't save when deleting
        }
        
        if (repository_->deleteProject(projectId)) {
            notifyProjectChanged("Project deleted: " + projectId);
            return true;
        } else {
            notifyError("Failed to delete project: " + projectId);
            return false;
        }
        
    } catch (const std::exception& e) {
        notifyError("Exception deleting project: " + std::string(e.what()));
        return false;
    }
}

std::string ProjectManager::getCurrentProjectId() const {
    return hasCurrentProject() ? currentProject_->getId() : "";
}

std::vector<ProjectInfo> ProjectManager::listProjects() const {
    try {
        return repository_->listProjects();
    } catch (const std::exception& e) {
        notifyError("Exception listing projects: " + std::string(e.what()));
        return {};
    }
}

std::vector<ProjectInfo> ProjectManager::listRecentProjects(size_t maxCount) const {
    try {
        return repository_->listRecentProjects(maxCount);
    } catch (const std::exception& e) {
        notifyError("Exception listing recent projects: " + std::string(e.what()));
        return {};
    }
}

std::vector<ProjectInfo> ProjectManager::searchProjects(const std::string& searchTerm) const {
    try {
        return repository_->searchProjects(searchTerm);
    } catch (const std::exception& e) {
        notifyError("Exception searching projects: " + std::string(e.what()));
        return {};
    }
}

std::optional<ProjectInfo> ProjectManager::getProjectInfo(const std::string& projectId) const {
    try {
        return repository_->getProjectInfo(projectId);
    } catch (const std::exception& e) {
        notifyError("Exception getting project info: " + std::string(e.what()));
        return std::nullopt;
    }
}

void ProjectManager::setAutoSaveConfig(const AutoSaveConfig& config) {
    autoSaveConfig_ = config;
    
    // Create backup directory if it doesn't exist
    if (!config.backupDirectory.empty()) {
        std::filesystem::create_directories(config.backupDirectory);
    }
}

void ProjectManager::setAutoSaveEnabled(bool enabled) {
    autoSaveConfig_.enabled = enabled;
    
    if (hasCurrentProject()) {
        if (enabled) {
            repository_->enableAutoSave(currentProject_->getId(), autoSaveConfig_.intervalSeconds);
        } else {
            repository_->disableAutoSave(currentProject_->getId());
        }
    }
}

bool ProjectManager::shouldAutoSave() const {
    if (!autoSaveConfig_.enabled || !hasCurrentProject() || !hasUnsavedChanges_) {
        return false;
    }
    
    auto now = std::chrono::system_clock::now();
    auto timeSinceLastAutoSave = std::chrono::duration_cast<std::chrono::seconds>(
        now - lastAutoSave_).count();
    
    return timeSinceLastAutoSave >= autoSaveConfig_.intervalSeconds;
}

bool ProjectManager::performAutoSave() {
    if (!shouldAutoSave()) {
        return true; // Nothing to do
    }
    
    try {
        if (saveProject()) {
            lastAutoSave_ = std::chrono::system_clock::now();
            notifyAutoSave("Auto-save completed for: " + currentProject_->getName());
            return true;
        } else {
            notifyError("Auto-save failed");
            return false;
        }
    } catch (const std::exception& e) {
        notifyError("Exception during auto-save: " + std::string(e.what()));
        return false;
    }
}

bool ProjectManager::forceAutoSave() {
    if (!hasCurrentProject()) {
        return false;
    }
    
    bool wasEnabled = autoSaveConfig_.enabled;
    autoSaveConfig_.enabled = true;
    hasUnsavedChanges_ = true; // Force save
    
    bool result = performAutoSave();
    autoSaveConfig_.enabled = wasEnabled;
    
    return result;
}

void ProjectManager::addTemplate(const ProjectTemplate& projectTemplate) {
    // Remove existing template with same ID
    removeTemplate(projectTemplate.id);
    templates_.push_back(projectTemplate);
}

bool ProjectManager::removeTemplate(const std::string& templateId) {
    auto it = std::find_if(templates_.begin(), templates_.end(),
        [&templateId](const ProjectTemplate& t) { return t.id == templateId; });
    
    if (it != templates_.end()) {
        templates_.erase(it);
        return true;
    }
    return false;
}

std::string ProjectManager::createTemplateFromProject(const std::string& templateName, 
                                                     const std::string& description) {
    if (!hasCurrentProject()) {
        notifyError("No current project to create template from");
        return "";
    }
    
    ProjectTemplate newTemplate;
    newTemplate.id = "template_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    newTemplate.name = templateName;
    newTemplate.description = description;
    newTemplate.defaultDimensions = currentProject_->getDimensions();
    
    // TODO: Extract object IDs for preloaded objects
    // This would require integration with the scene manager
    
    addTemplate(newTemplate);
    return newTemplate.id;
}

std::string ProjectManager::duplicateProject(const std::string& projectId, const std::string& newName) {
    try {
        // Load the source project
        auto sourceProject = repository_->loadProject(projectId);
        if (!sourceProject || !sourceProject.value()) {
            notifyError("Failed to load source project for duplication");
            return "";
        }
        
        // Create metadata for new project
        ProjectMetadata metadata;
        metadata.name = newName;
        metadata.description = sourceProject.value()->getDescription() + " (Copy)";
        metadata.roomWidth = sourceProject.value()->getDimensions().width;
        metadata.roomHeight = sourceProject.value()->getDimensions().height;
        metadata.roomDepth = sourceProject.value()->getDimensions().depth;
        
        // Create new project
        std::string newProjectId = repository_->createProject(metadata);
        if (newProjectId.empty()) {
            notifyError("Failed to create duplicate project");
            return "";
        }
        
        // Copy project data
        sourceProject.value()->setId(newProjectId);
        sourceProject.value()->setName(newName);
        
        if (repository_->saveProject(*sourceProject.value())) {
            notifyProjectChanged("Project duplicated: " + newName);
            return newProjectId;
        } else {
            repository_->deleteProject(newProjectId);
            notifyError("Failed to save duplicated project");
            return "";
        }
        
    } catch (const std::exception& e) {
        notifyError("Exception duplicating project: " + std::string(e.what()));
        return "";
    }
}

bool ProjectManager::exportProject(const std::string& projectId, const std::string& filePath) {
    try {
        return repository_->exportProject(projectId, filePath);
    } catch (const std::exception& e) {
        notifyError("Exception exporting project: " + std::string(e.what()));
        return false;
    }
}

std::optional<std::string> ProjectManager::importProject(const std::string& filePath) {
    try {
        auto result = repository_->importProject(filePath);
        if (result) {
            notifyProjectChanged("Project imported from: " + filePath);
        }
        return result;
    } catch (const std::exception& e) {
        notifyError("Exception importing project: " + std::string(e.what()));
        return std::nullopt;
    }
}

bool ProjectManager::createBackup(const std::string& backupName) {
    if (!hasCurrentProject()) {
        notifyError("No current project to backup");
        return false;
    }
    
    try {
        std::string filename = generateBackupFilename(currentProject_->getId(), backupName);
        std::string backupPath = autoSaveConfig_.backupDirectory + "/" + filename;
        
        if (repository_->exportProject(currentProject_->getId(), backupPath)) {
            notifyProjectChanged("Backup created: " + filename);
            return true;
        } else {
            notifyError("Failed to create backup");
            return false;
        }
        
    } catch (const std::exception& e) {
        notifyError("Exception creating backup: " + std::string(e.what()));
        return false;
    }
}

bool ProjectManager::restoreFromBackup(const std::string& backupPath) {
    try {
        auto projectId = repository_->importProject(backupPath);
        if (projectId) {
            notifyProjectChanged("Project restored from backup: " + backupPath);
            return loadProject(projectId.value());
        } else {
            notifyError("Failed to restore from backup");
            return false;
        }
    } catch (const std::exception& e) {
        notifyError("Exception restoring from backup: " + std::string(e.what()));
        return false;
    }
}

void ProjectManager::markAsModified() {
    hasUnsavedChanges_ = true;
    if (hasCurrentProject()) {
        currentProject_->updateTimestamp();
    }
}

void ProjectManager::markAsSaved() {
    hasUnsavedChanges_ = false;
    lastSave_ = std::chrono::system_clock::now();
}

std::chrono::seconds ProjectManager::getTimeSinceLastSave() const {
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(now - lastSave_);
}

std::chrono::seconds ProjectManager::getTimeSinceLastAutoSave() const {
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(now - lastAutoSave_);
}

ProjectManager::ProjectStatistics ProjectManager::getStatistics() const {
    ProjectStatistics stats;
    
    try {
        auto projects = repository_->listProjects();
        stats.totalProjects = projects.size();
        stats.totalObjects = 0;
        stats.totalValue = 0.0;
        stats.lastActivity = std::chrono::system_clock::time_point::min();
        
        // Calculate aggregated statistics
        for (const auto& projectInfo : projects) {
            stats.totalObjects += projectInfo.objectCount;
            
            // Parse timestamp for last activity
            // TODO: Implement proper timestamp parsing
        }
        
        // TODO: Calculate most used category and total value
        // This would require integration with CatalogService
        
    } catch (const std::exception& e) {
        notifyError("Exception calculating statistics: " + std::string(e.what()));
    }
    
    return stats;
}

void ProjectManager::cleanup() {
    try {
        // Clean up old backups
        if (!autoSaveConfig_.backupDirectory.empty() && 
            std::filesystem::exists(autoSaveConfig_.backupDirectory)) {
            
            std::vector<std::filesystem::directory_entry> backupFiles;
            for (const auto& entry : std::filesystem::directory_iterator(autoSaveConfig_.backupDirectory)) {
                if (entry.is_regular_file()) {
                    backupFiles.push_back(entry);
                }
            }
            
            // Sort by modification time (newest first)
            std::sort(backupFiles.begin(), backupFiles.end(),
                [](const auto& a, const auto& b) {
                    return std::filesystem::last_write_time(a) > std::filesystem::last_write_time(b);
                });
            
            // Remove old backups beyond maxBackups
            if (backupFiles.size() > static_cast<size_t>(autoSaveConfig_.maxBackups)) {
                for (size_t i = autoSaveConfig_.maxBackups; i < backupFiles.size(); ++i) {
                    std::filesystem::remove(backupFiles[i]);
                }
            }
        }
        
        // Vacuum repository
        repository_->vacuum();
        
    } catch (const std::exception& e) {
        notifyError("Exception during cleanup: " + std::string(e.what()));
    }
}

bool ProjectManager::validateRepository() {
    try {
        // Basic validation - check if we can list projects
        auto projects = repository_->listProjects();
        
        // TODO: Add more comprehensive validation
        // - Check database integrity
        // - Validate project files exist
        // - Check for orphaned records
        
        return true;
        
    } catch (const std::exception& e) {
        notifyError("Repository validation failed: " + std::string(e.what()));
        return false;
    }
}

void ProjectManager::initializeDefaultTemplates() {
    // Small kitchen template
    ProjectTemplate smallKitchen;
    smallKitchen.id = "template_small_kitchen";
    smallKitchen.name = "Small Kitchen";
    smallKitchen.description = "Compact kitchen layout for small spaces";
    smallKitchen.defaultDimensions = RoomDimensions(3.0, 2.5, 2.0); // 3m x 2.5m x 2m
    
    // Medium kitchen template
    ProjectTemplate mediumKitchen;
    mediumKitchen.id = "template_medium_kitchen";
    mediumKitchen.name = "Medium Kitchen";
    mediumKitchen.description = "Standard kitchen layout for average homes";
    mediumKitchen.defaultDimensions = RoomDimensions(4.0, 3.0, 2.5); // 4m x 3m x 2.5m
    
    // Large kitchen template
    ProjectTemplate largeKitchen;
    largeKitchen.id = "template_large_kitchen";
    largeKitchen.name = "Large Kitchen";
    largeKitchen.description = "Spacious kitchen layout with island";
    largeKitchen.defaultDimensions = RoomDimensions(6.0, 4.0, 2.5); // 6m x 4m x 2.5m
    
    templates_ = {smallKitchen, mediumKitchen, largeKitchen};
}

void ProjectManager::notifyProjectChanged(const std::string& message) {
    if (projectChangedCallback_) {
        projectChangedCallback_(message);
    }
}

void ProjectManager::notifyAutoSave(const std::string& message) {
    if (autoSaveCallback_) {
        autoSaveCallback_(message);
    }
}

void ProjectManager::notifyError(const std::string& error) const {
    if (errorCallback_) {
        errorCallback_(error);
    }
}

void ProjectManager::updateTimestamps() {
    auto now = std::chrono::system_clock::now();
    lastSave_ = now;
    lastAutoSave_ = now;
}

std::string ProjectManager::generateBackupFilename(const std::string& projectId, 
                                                  const std::string& suffix) const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << "backup_" << projectId << "_" 
       << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
    
    if (!suffix.empty()) {
        ss << "_" << suffix;
    }
    
    ss << ".kcad";
    return ss.str();
}

} // namespace Services
} // namespace KitchenCAD