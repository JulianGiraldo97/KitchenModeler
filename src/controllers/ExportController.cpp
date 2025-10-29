#include "ExportController.h"
#include "../utils/Logger.h"
#include <filesystem>
#include <algorithm>
#include <chrono>

namespace KitchenCAD {
namespace Controllers {

ExportController::ExportController(std::unique_ptr<ICADExporter> cadExporter)
    : cadExporter_(std::move(cadExporter))
    , currentProject_(nullptr)
{
    setupCADExporterCallbacks();
}

std::vector<CADFormat> ExportController::getSupportedFormats() const {
    if (cadExporter_) {
        return cadExporter_->getSupportedFormats();
    }
    
    return {};
}

bool ExportController::isFormatSupported(CADFormat format) const {
    if (cadExporter_) {
        return cadExporter_->isFormatSupported(format);
    }
    
    return false;
}

std::string ExportController::getFormatExtension(CADFormat format) const {
    if (cadExporter_) {
        return cadExporter_->getFormatExtension(format);
    }
    
    return "";
}

std::string ExportController::getFormatDescription(CADFormat format) const {
    if (cadExporter_) {
        return cadExporter_->getFormatDescription(format);
    }
    
    return "";
}

std::vector<ExportController::FormatInfo> ExportController::getAllFormatInfo() const {
    std::vector<FormatInfo> formatInfo;
    
    // List all possible formats
    std::vector<CADFormat> allFormats = {
        CADFormat::STEP, CADFormat::IGES, CADFormat::STL, CADFormat::OBJ,
        CADFormat::PLY, CADFormat::COLLADA, CADFormat::GLTF, CADFormat::X3D
    };
    
    for (CADFormat format : allFormats) {
        FormatInfo info;
        info.format = format;
        info.extension = getFormatExtension(format);
        info.description = getFormatDescription(format);
        info.supported = isFormatSupported(format);
        formatInfo.push_back(info);
    }
    
    return formatInfo;
}

ExportResult ExportController::exportObject(const std::string& objectId,
                                          const std::string& filePath,
                                          CADFormat format,
                                          ExportQuality quality) {
    ExportOptions options = createExportOptions(format, quality);
    return exportObject(objectId, filePath, options);
}

ExportResult ExportController::exportObject(const std::string& objectId,
                                          const std::string& filePath,
                                          const ExportOptions& options) {
    if (!validateExportPreconditions(filePath)) {
        return ExportResult(false, "Export validation failed");
    }
    
    if (!currentProject_) {
        notifyError("No project loaded");
        return ExportResult(false, "No project loaded");
    }
    
    const SceneObject* object = currentProject_->getObject(objectId);
    if (!object) {
        notifyError("Object not found: " + objectId);
        return ExportResult(false, "Object not found: " + objectId);
    }
    
    try {
        notifyProgress(0.0, "Starting object export...");
        
        ExportResult result = cadExporter_->exportObject(*object, filePath, options);
        
        recordExport(result, options.format, options.quality);
        
        if (result.success) {
            notifyProgress(1.0, "Export completed successfully");
            notifyExportCompleted(result);
        } else {
            notifyError("Export failed: " + result.message);
        }
        
        return result;
        
    } catch (const std::exception& e) {
        std::string error = "Export failed: " + std::string(e.what());
        notifyError(error);
        return ExportResult(false, error);
    }
}

ExportResult ExportController::exportObjects(const std::vector<std::string>& objectIds,
                                           const std::string& filePath,
                                           CADFormat format,
                                           ExportQuality quality) {
    ExportOptions options = createExportOptions(format, quality);
    return exportObjects(objectIds, filePath, options);
}

ExportResult ExportController::exportObjects(const std::vector<std::string>& objectIds,
                                           const std::string& filePath,
                                           const ExportOptions& options) {
    if (!validateExportPreconditions(filePath)) {
        return ExportResult(false, "Export validation failed");
    }
    
    if (!currentProject_) {
        notifyError("No project loaded");
        return ExportResult(false, "No project loaded");
    }
    
    if (objectIds.empty()) {
        notifyError("No objects specified for export");
        return ExportResult(false, "No objects specified for export");
    }
    
    try {
        notifyProgress(0.0, "Collecting objects for export...");
        
        auto objects = getObjectsById(objectIds);
        if (objects.empty()) {
            notifyError("No valid objects found for export");
            return ExportResult(false, "No valid objects found for export");
        }
        
        notifyProgress(0.2, "Starting objects export...");
        
        ExportResult result = cadExporter_->exportObjects(objects, filePath, options);
        
        recordExport(result, options.format, options.quality);
        
        if (result.success) {
            notifyProgress(1.0, "Export completed successfully");
            notifyExportCompleted(result);
        } else {
            notifyError("Export failed: " + result.message);
        }
        
        return result;
        
    } catch (const std::exception& e) {
        std::string error = "Export failed: " + std::string(e.what());
        notifyError(error);
        return ExportResult(false, error);
    }
}

ExportResult ExportController::exportSelectedObjects(const std::string& filePath,
                                                   CADFormat format,
                                                   ExportQuality quality) {
    // This would need to get selected objects from a scene manager or selection service
    // For now, return an error indicating this functionality needs integration
    notifyError("Selected objects export requires scene manager integration");
    return ExportResult(false, "Selected objects export not implemented");
}

ExportResult ExportController::exportProject(const std::string& filePath,
                                           CADFormat format,
                                           ExportQuality quality) {
    ExportOptions options = createExportOptions(format, quality);
    return exportProject(filePath, options);
}

ExportResult ExportController::exportProject(const std::string& filePath,
                                           const ExportOptions& options) {
    if (!validateExportPreconditions(filePath)) {
        return ExportResult(false, "Export validation failed");
    }
    
    if (!currentProject_) {
        notifyError("No project loaded");
        return ExportResult(false, "No project loaded");
    }
    
    try {
        notifyProgress(0.0, "Starting project export...");
        
        ExportResult result = cadExporter_->exportProject(*currentProject_, filePath, options);
        
        recordExport(result, options.format, options.quality);
        
        if (result.success) {
            notifyProgress(1.0, "Project export completed successfully");
            notifyExportCompleted(result);
        } else {
            notifyError("Project export failed: " + result.message);
        }
        
        return result;
        
    } catch (const std::exception& e) {
        std::string error = "Project export failed: " + std::string(e.what());
        notifyError(error);
        return ExportResult(false, error);
    }
}

ExportResult ExportController::exportByCategory(const std::string& category,
                                              const std::string& filePath,
                                              CADFormat format,
                                              ExportQuality quality) {
    ExportOptions options = createExportOptions(format, quality);
    return exportByCategory(category, filePath, options);
}

ExportResult ExportController::exportByCategory(const std::string& category,
                                              const std::string& filePath,
                                              const ExportOptions& options) {
    if (!validateExportPreconditions(filePath)) {
        return ExportResult(false, "Export validation failed");
    }
    
    if (!currentProject_) {
        notifyError("No project loaded");
        return ExportResult(false, "No project loaded");
    }
    
    if (category.empty()) {
        notifyError("Category cannot be empty");
        return ExportResult(false, "Category cannot be empty");
    }
    
    try {
        notifyProgress(0.0, "Starting category export: " + category);
        
        ExportResult result = cadExporter_->exportByCategory(*currentProject_, category, filePath, options);
        
        recordExport(result, options.format, options.quality);
        
        if (result.success) {
            notifyProgress(1.0, "Category export completed successfully");
            notifyExportCompleted(result);
        } else {
            notifyError("Category export failed: " + result.message);
        }
        
        return result;
        
    } catch (const std::exception& e) {
        std::string error = "Category export failed: " + std::string(e.what());
        notifyError(error);
        return ExportResult(false, error);
    }
}

ExportResult ExportController::exportRegion(const BoundingBox& region,
                                          const std::string& filePath,
                                          CADFormat format,
                                          ExportQuality quality) {
    if (!validateExportPreconditions(filePath)) {
        return ExportResult(false, "Export validation failed");
    }
    
    if (!currentProject_) {
        notifyError("No project loaded");
        return ExportResult(false, "No project loaded");
    }
    
    try {
        notifyProgress(0.0, "Collecting objects in region...");
        
        auto objects = getObjectsInRegion(region);
        if (objects.empty()) {
            notifyError("No objects found in specified region");
            return ExportResult(false, "No objects found in specified region");
        }
        
        ExportOptions options = createExportOptions(format, quality);
        
        notifyProgress(0.2, "Starting region export...");
        
        ExportResult result = cadExporter_->exportObjects(objects, filePath, options);
        
        recordExport(result, options.format, options.quality);
        
        if (result.success) {
            notifyProgress(1.0, "Region export completed successfully");
            notifyExportCompleted(result);
        } else {
            notifyError("Region export failed: " + result.message);
        }
        
        return result;
        
    } catch (const std::exception& e) {
        std::string error = "Region export failed: " + std::string(e.what());
        notifyError(error);
        return ExportResult(false, error);
    }
}

std::vector<ExportResult> ExportController::exportToMultipleFormats(const std::string& baseFilePath,
                                                                   const std::vector<CADFormat>& formats,
                                                                   ExportQuality quality) {
    std::vector<ExportResult> results;
    
    if (!currentProject_) {
        notifyError("No project loaded");
        results.push_back(ExportResult(false, "No project loaded"));
        return results;
    }
    
    std::filesystem::path basePath(baseFilePath);
    std::string baseName = basePath.stem().string();
    std::string directory = basePath.parent_path().string();
    
    for (size_t i = 0; i < formats.size(); ++i) {
        CADFormat format = formats[i];
        
        notifyProgress(static_cast<double>(i) / formats.size(), 
                      "Exporting to " + getFormatDescription(format));
        
        std::string extension = getFormatExtension(format);
        std::string filePath = directory + "/" + baseName + extension;
        
        ExportResult result = exportProject(filePath, format, quality);
        results.push_back(result);
        
        if (!result.success) {
            notifyError("Failed to export to " + getFormatDescription(format));
        }
    }
    
    notifyProgress(1.0, "Multiple format export completed");
    return results;
}

std::vector<ExportResult> ExportController::exportCategoriesSeparately(const std::string& baseDirectory,
                                                                      CADFormat format,
                                                                      ExportQuality quality) {
    std::vector<ExportResult> results;
    
    if (!currentProject_) {
        notifyError("No project loaded");
        results.push_back(ExportResult(false, "No project loaded"));
        return results;
    }
    
    // Create export directory
    if (!createExportDirectory(baseDirectory)) {
        notifyError("Failed to create export directory: " + baseDirectory);
        results.push_back(ExportResult(false, "Failed to create export directory"));
        return results;
    }
    
    // Get all categories with objects
    std::set<std::string> categories;
    for (const auto& object : currentProject_->getObjects()) {
        if (object) {
            // Would need to get category from catalog item
            // This is a placeholder implementation
            categories.insert("default_category");
        }
    }
    
    std::string extension = getFormatExtension(format);
    
    size_t categoryIndex = 0;
    for (const auto& category : categories) {
        notifyProgress(static_cast<double>(categoryIndex) / categories.size(), 
                      "Exporting category: " + category);
        
        std::string filePath = baseDirectory + "/" + category + extension;
        ExportResult result = exportByCategory(category, filePath, format, quality);
        results.push_back(result);
        
        if (!result.success) {
            notifyError("Failed to export category: " + category);
        }
        
        categoryIndex++;
    }
    
    notifyProgress(1.0, "Category export completed");
    return results;
}

std::vector<ExportResult> ExportController::exportObjectsSeparately(const std::string& baseDirectory,
                                                                   CADFormat format,
                                                                   ExportQuality quality) {
    std::vector<ExportResult> results;
    
    if (!currentProject_) {
        notifyError("No project loaded");
        results.push_back(ExportResult(false, "No project loaded"));
        return results;
    }
    
    // Create export directory
    if (!createExportDirectory(baseDirectory)) {
        notifyError("Failed to create export directory: " + baseDirectory);
        results.push_back(ExportResult(false, "Failed to create export directory"));
        return results;
    }
    
    const auto& objects = currentProject_->getObjects();
    std::string extension = getFormatExtension(format);
    
    for (size_t i = 0; i < objects.size(); ++i) {
        const auto& object = objects[i];
        if (!object) continue;
        
        notifyProgress(static_cast<double>(i) / objects.size(), 
                      "Exporting object: " + object->getId());
        
        std::string filePath = baseDirectory + "/" + object->getId() + extension;
        ExportResult result = exportObject(object->getId(), filePath, format, quality);
        results.push_back(result);
        
        if (!result.success) {
            notifyError("Failed to export object: " + object->getId());
        }
    }
    
    notifyProgress(1.0, "Individual object export completed");
    return results;
}

ExportResult ExportController::exportAsImage(const std::string& filePath,
                                           int width, int height,
                                           const std::string& format) {
    // This would require integration with a render engine
    // For now, return a placeholder implementation
    notifyError("Image export requires render engine integration");
    return ExportResult(false, "Image export not implemented");
}

std::vector<ExportResult> ExportController::exportMultipleViews(const std::vector<ViewExport>& views,
                                                               const std::string& format) {
    std::vector<ExportResult> results;
    
    // This would require integration with a render engine
    // For now, return placeholder results
    for (const auto& view : views) {
        results.push_back(ExportResult(false, "Multiple view export not implemented"));
    }
    
    return results;
}

std::vector<ExportResult> ExportController::exportTurntable(const std::string& baseDirectory,
                                                          int frameCount,
                                                          int width, int height,
                                                          const std::string& format) {
    std::vector<ExportResult> results;
    
    // This would require integration with a render engine
    // For now, return placeholder results
    for (int i = 0; i < frameCount; ++i) {
        results.push_back(ExportResult(false, "Turntable export not implemented"));
    }
    
    return results;
}

ExportResult ExportController::exportBOMAsCSV(const std::string& filePath) {
    if (!currentProject_) {
        notifyError("No project loaded");
        return ExportResult(false, "No project loaded");
    }
    
    try {
        // This would use the BOM generator service
        // For now, return a placeholder implementation
        notifyError("BOM CSV export requires BOM generator integration");
        return ExportResult(false, "BOM CSV export not implemented");
        
    } catch (const std::exception& e) {
        std::string error = "BOM CSV export failed: " + std::string(e.what());
        notifyError(error);
        return ExportResult(false, error);
    }
}

ExportResult ExportController::exportBOMAsJSON(const std::string& filePath) {
    if (!currentProject_) {
        notifyError("No project loaded");
        return ExportResult(false, "No project loaded");
    }
    
    try {
        // This would use the BOM generator service
        // For now, return a placeholder implementation
        notifyError("BOM JSON export requires BOM generator integration");
        return ExportResult(false, "BOM JSON export not implemented");
        
    } catch (const std::exception& e) {
        std::string error = "BOM JSON export failed: " + std::string(e.what());
        notifyError(error);
        return ExportResult(false, error);
    }
}

ExportResult ExportController::exportProjectReport(const std::string& filePath,
                                                  bool includeImages,
                                                  bool includeBOM,
                                                  bool includeSpecifications) {
    if (!currentProject_) {
        notifyError("No project loaded");
        return ExportResult(false, "No project loaded");
    }
    
    try {
        // This would require a report generator
        // For now, return a placeholder implementation
        notifyError("Project report export requires report generator integration");
        return ExportResult(false, "Project report export not implemented");
        
    } catch (const std::exception& e) {
        std::string error = "Project report export failed: " + std::string(e.what());
        notifyError(error);
        return ExportResult(false, error);
    }
}

ExportResult ExportController::exportTechnicalDrawings(const std::string& filePath,
                                                     bool includeTopView,
                                                     bool includeFrontView,
                                                     bool includeSideView,
                                                     bool includeIsometric) {
    if (!currentProject_) {
        notifyError("No project loaded");
        return ExportResult(false, "No project loaded");
    }
    
    try {
        // This would require a technical drawing generator
        // For now, return a placeholder implementation
        notifyError("Technical drawings export requires drawing generator integration");
        return ExportResult(false, "Technical drawings export not implemented");
        
    } catch (const std::exception& e) {
        std::string error = "Technical drawings export failed: " + std::string(e.what());
        notifyError(error);
        return ExportResult(false, error);
    }
}

bool ExportController::validateExportPath(const std::string& filePath, CADFormat format) const {
    if (cadExporter_) {
        return cadExporter_->validateExportPath(filePath, format);
    }
    
    return false;
}

bool ExportController::canExportObjects(const std::vector<std::string>& objectIds, CADFormat format) const {
    if (!currentProject_ || !cadExporter_) {
        return false;
    }
    
    for (const auto& objectId : objectIds) {
        const SceneObject* object = currentProject_->getObject(objectId);
        if (!object || !cadExporter_->canExportObject(*object, format)) {
            return false;
        }
    }
    
    return true;
}

size_t ExportController::estimateFileSize(const std::vector<std::string>& objectIds, 
                                         const ExportOptions& options) const {
    if (!currentProject_ || !cadExporter_) {
        return 0;
    }
    
    return cadExporter_->estimateFileSize(*currentProject_, options);
}

double ExportController::estimateExportTime(const std::vector<std::string>& objectIds, 
                                          const ExportOptions& options) const {
    if (!currentProject_ || !cadExporter_) {
        return 0.0;
    }
    
    return cadExporter_->estimateExportTime(*currentProject_, options);
}

void ExportController::setDefaultExportOptions(const ExportOptions& options) {
    if (cadExporter_) {
        cadExporter_->setDefaultOptions(options);
    }
}

ExportOptions ExportController::getDefaultExportOptions() const {
    if (cadExporter_) {
        return cadExporter_->getDefaultOptions();
    }
    
    return ExportOptions();
}

ExportOptions ExportController::createExportOptions(CADFormat format, 
                                                   ExportQuality quality,
                                                   bool includeTextures,
                                                   bool includeMaterials,
                                                   bool includeMetadata) const {
    ExportOptions options;
    options.format = format;
    options.quality = quality;
    options.includeTextures = includeTextures;
    options.includeMaterials = includeMaterials;
    options.includeMetadata = includeMetadata;
    
    return options;
}

std::string ExportController::suggestFileName(CADFormat format, const std::string& baseName) const {
    std::string name = baseName;
    
    if (name.empty() && currentProject_) {
        name = currentProject_->getName();
    }
    
    if (name.empty()) {
        name = "export";
    }
    
    if (cadExporter_) {
        return cadExporter_->suggestFileName(*currentProject_, format);
    }
    
    return name + getFormatExtension(format);
}

bool ExportController::isValidFileName(const std::string& fileName, CADFormat format) const {
    if (cadExporter_) {
        return cadExporter_->isValidFileName(fileName, format);
    }
    
    return !fileName.empty();
}

std::string ExportController::generateUniqueFileName(const std::string& baseFilePath) const {
    std::filesystem::path path(baseFilePath);
    std::string baseName = path.stem().string();
    std::string extension = path.extension().string();
    std::string directory = path.parent_path().string();
    
    std::string uniquePath = baseFilePath;
    int counter = 1;
    
    while (std::filesystem::exists(uniquePath)) {
        uniquePath = directory + "/" + baseName + "_" + std::to_string(counter) + extension;
        counter++;
    }
    
    return uniquePath;
}

std::string ExportController::getDefaultExportDirectory() const {
    if (currentProject_) {
        // Create exports directory relative to project
        return "exports/" + currentProject_->getName();
    }
    
    return "exports";
}

bool ExportController::createExportDirectory(const std::string& directory) const {
    try {
        std::filesystem::create_directories(directory);
        return true;
    } catch (const std::exception& e) {
        notifyError("Failed to create directory: " + std::string(e.what()));
        return false;
    }
}

void ExportController::clearCallbacks() {
    progressCallback_ = nullptr;
    exportCompletedCallback_ = nullptr;
    errorCallback_ = nullptr;
}

std::string ExportController::getLastError() const {
    if (cadExporter_) {
        return cadExporter_->getLastError();
    }
    
    return lastError_;
}

void ExportController::clearLastError() {
    if (cadExporter_) {
        cadExporter_->clearLastError();
    }
    
    lastError_.clear();
}

std::vector<ExportController::ExportHistoryEntry> ExportController::getExportHistory(size_t maxEntries) const {
    if (exportHistory_.size() <= maxEntries) {
        return exportHistory_;
    }
    
    return std::vector<ExportHistoryEntry>(
        exportHistory_.end() - maxEntries, 
        exportHistory_.end()
    );
}

void ExportController::clearExportHistory() {
    exportHistory_.clear();
}

ExportController::ExportStatistics ExportController::getExportStatistics() const {
    ExportStatistics stats;
    
    stats.totalExports = exportHistory_.size();
    
    for (const auto& entry : exportHistory_) {
        if (entry.success) {
            stats.successfulExports++;
            stats.totalBytesExported += entry.fileSize;
            stats.averageExportTime += entry.exportTime;
        } else {
            stats.failedExports++;
        }
        
        stats.formatCounts[entry.format]++;
    }
    
    if (stats.successfulExports > 0) {
        stats.averageExportTime /= stats.successfulExports;
    }
    
    return stats;
}

std::vector<const SceneObject*> ExportController::getObjectsById(const std::vector<std::string>& objectIds) const {
    std::vector<const SceneObject*> objects;
    
    if (!currentProject_) {
        return objects;
    }
    
    for (const auto& objectId : objectIds) {
        const SceneObject* object = currentProject_->getObject(objectId);
        if (object) {
            objects.push_back(object);
        }
    }
    
    return objects;
}

std::vector<const SceneObject*> ExportController::getObjectsByCategory(const std::string& category) const {
    std::vector<const SceneObject*> objects;
    
    if (!currentProject_) {
        return objects;
    }
    
    // This would need catalog integration to determine object categories
    // For now, return all objects as placeholder
    for (const auto& object : currentProject_->getObjects()) {
        if (object) {
            objects.push_back(object.get());
        }
    }
    
    return objects;
}

std::vector<const SceneObject*> ExportController::getObjectsInRegion(const BoundingBox& region) const {
    std::vector<const SceneObject*> objects;
    
    if (!currentProject_) {
        return objects;
    }
    
    for (const auto& object : currentProject_->getObjects()) {
        if (object) {
            // Check if object intersects with region
            // This is a simplified check using object position
            Transform3D transform = object->getTransform();
            Point3D position = transform.translation;
            
            if (position.x >= region.min.x && position.x <= region.max.x &&
                position.y >= region.min.y && position.y <= region.max.y &&
                position.z >= region.min.z && position.z <= region.max.z) {
                objects.push_back(object.get());
            }
        }
    }
    
    return objects;
}

void ExportController::recordExport(const ExportResult& result, CADFormat format, ExportQuality quality) {
    ExportHistoryEntry entry;
    entry.filePath = result.filePath;
    entry.format = format;
    entry.quality = quality;
    entry.timestamp = std::chrono::system_clock::now();
    entry.success = result.success;
    entry.fileSize = result.fileSize;
    entry.exportTime = result.exportTime;
    entry.errorMessage = result.message;
    
    exportHistory_.push_back(entry);
    
    // Keep history size manageable
    const size_t maxHistorySize = 1000;
    if (exportHistory_.size() > maxHistorySize) {
        exportHistory_.erase(exportHistory_.begin(), 
                           exportHistory_.begin() + (exportHistory_.size() - maxHistorySize));
    }
}

void ExportController::notifyProgress(double progress, const std::string& status) {
    if (progressCallback_) {
        progressCallback_(progress, status);
    }
}

void ExportController::notifyExportCompleted(const ExportResult& result) {
    if (exportCompletedCallback_) {
        exportCompletedCallback_(result);
    }
}

void ExportController::notifyError(const std::string& error) {
    lastError_ = error;
    
    if (errorCallback_) {
        errorCallback_(error);
    }
}

bool ExportController::validateExportPreconditions(const std::string& filePath) const {
    if (filePath.empty()) {
        notifyError("File path cannot be empty");
        return false;
    }
    
    if (!currentProject_) {
        notifyError("No project loaded");
        return false;
    }
    
    if (!cadExporter_) {
        notifyError("CAD exporter not available");
        return false;
    }
    
    return true;
}

void ExportController::setupCADExporterCallbacks() {
    if (cadExporter_) {
        cadExporter_->setProgressCallback([this](double progress, const std::string& status) {
            notifyProgress(progress, status);
        });
    }
}

} // namespace Controllers
} // namespace KitchenCAD