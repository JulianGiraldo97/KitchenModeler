#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include "../interfaces/ICADExporter.h"
#include "../models/Project.h"
#include "../geometry/BoundingBox.h"
#include "../core/Camera3D.h"
#include <chrono>
#include <unordered_map>

namespace KitchenCAD {
namespace Controllers {

using namespace Models;

/**
 * @brief Export controller for managing export operations
 * 
 * This controller handles all export-related operations including CAD exports,
 * image exports, and report generation. Implements requirements 9.1, 9.2, 9.3, 9.4, 9.5.
 */
class ExportController {
private:
    std::unique_ptr<ICADExporter> cadExporter_;
    Project* currentProject_;
    
    // Callbacks
    std::function<void(double, const std::string&)> progressCallback_;
    std::function<void(const ExportResult&)> exportCompletedCallback_;
    std::function<void(const std::string&)> errorCallback_;

public:
    /**
     * @brief Constructor
     */
    explicit ExportController(std::unique_ptr<ICADExporter> cadExporter);
    
    /**
     * @brief Destructor
     */
    ~ExportController() = default;
    
    // Project management
    
    /**
     * @brief Set current project
     */
    void setCurrentProject(Project* project) { currentProject_ = project; }
    
    /**
     * @brief Get current project
     */
    Project* getCurrentProject() const { return currentProject_; }
    
    /**
     * @brief Check if project is loaded
     */
    bool hasProject() const { return currentProject_ != nullptr; }
    
    // Format support queries
    
    /**
     * @brief Get supported export formats
     */
    std::vector<CADFormat> getSupportedFormats() const;
    
    /**
     * @brief Check if format is supported
     */
    bool isFormatSupported(CADFormat format) const;
    
    /**
     * @brief Get format file extension
     */
    std::string getFormatExtension(CADFormat format) const;
    
    /**
     * @brief Get format description
     */
    std::string getFormatDescription(CADFormat format) const;
    
    /**
     * @brief Get all format information
     */
    struct FormatInfo {
        CADFormat format;
        std::string extension;
        std::string description;
        bool supported;
    };
    std::vector<FormatInfo> getAllFormatInfo() const;
    
    // Single object export
    
    /**
     * @brief Export single object by ID
     */
    ExportResult exportObject(const std::string& objectId,
                            const std::string& filePath,
                            CADFormat format = CADFormat::STEP,
                            ExportQuality quality = ExportQuality::Standard);
    
    /**
     * @brief Export single object with custom options
     */
    ExportResult exportObject(const std::string& objectId,
                            const std::string& filePath,
                            const ExportOptions& options);
    
    // Multiple objects export
    
    /**
     * @brief Export multiple objects by IDs
     */
    ExportResult exportObjects(const std::vector<std::string>& objectIds,
                             const std::string& filePath,
                             CADFormat format = CADFormat::STEP,
                             ExportQuality quality = ExportQuality::Standard);
    
    /**
     * @brief Export multiple objects with custom options
     */
    ExportResult exportObjects(const std::vector<std::string>& objectIds,
                             const std::string& filePath,
                             const ExportOptions& options);
    
    /**
     * @brief Export selected objects
     */
    ExportResult exportSelectedObjects(const std::string& filePath,
                                     CADFormat format = CADFormat::STEP,
                                     ExportQuality quality = ExportQuality::Standard);
    
    // Full project export
    
    /**
     * @brief Export entire project
     */
    ExportResult exportProject(const std::string& filePath,
                             CADFormat format = CADFormat::STEP,
                             ExportQuality quality = ExportQuality::Standard);
    
    /**
     * @brief Export project with custom options
     */
    ExportResult exportProject(const std::string& filePath,
                             const ExportOptions& options);
    
    // Selective export
    
    /**
     * @brief Export objects by category
     */
    ExportResult exportByCategory(const std::string& category,
                                const std::string& filePath,
                                CADFormat format = CADFormat::STEP,
                                ExportQuality quality = ExportQuality::Standard);
    
    /**
     * @brief Export objects by category with custom options
     */
    ExportResult exportByCategory(const std::string& category,
                                const std::string& filePath,
                                const ExportOptions& options);
    
    /**
     * @brief Export objects within bounding box
     */
    ExportResult exportRegion(const BoundingBox& region,
                            const std::string& filePath,
                            CADFormat format = CADFormat::STEP,
                            ExportQuality quality = ExportQuality::Standard);
    
    // Batch export operations
    
    /**
     * @brief Export project to multiple formats
     */
    std::vector<ExportResult> exportToMultipleFormats(const std::string& baseFilePath,
                                                     const std::vector<CADFormat>& formats,
                                                     ExportQuality quality = ExportQuality::Standard);
    
    /**
     * @brief Export each category separately
     */
    std::vector<ExportResult> exportCategoriesSeparately(const std::string& baseDirectory,
                                                        CADFormat format = CADFormat::STEP,
                                                        ExportQuality quality = ExportQuality::Standard);
    
    /**
     * @brief Export each object separately
     */
    std::vector<ExportResult> exportObjectsSeparately(const std::string& baseDirectory,
                                                     CADFormat format = CADFormat::STEP,
                                                     ExportQuality quality = ExportQuality::Standard);
    
    // Image and rendering export
    
    /**
     * @brief Export project as image
     */
    ExportResult exportAsImage(const std::string& filePath,
                             int width = 1920, int height = 1080,
                             const std::string& format = "PNG");
    
    /**
     * @brief Export multiple views as images
     */
    struct ViewExport {
        std::string name;
        std::string filePath;
        Camera3D camera;
        int width = 1920;
        int height = 1080;
    };
    std::vector<ExportResult> exportMultipleViews(const std::vector<ViewExport>& views,
                                                 const std::string& format = "PNG");
    
    /**
     * @brief Export 360-degree turntable images
     */
    std::vector<ExportResult> exportTurntable(const std::string& baseDirectory,
                                            int frameCount = 36,
                                            int width = 1920, int height = 1080,
                                            const std::string& format = "PNG");
    
    // Report and documentation export
    
    /**
     * @brief Export bill of materials as CSV
     */
    ExportResult exportBOMAsCSV(const std::string& filePath);
    
    /**
     * @brief Export bill of materials as JSON
     */
    ExportResult exportBOMAsJSON(const std::string& filePath);
    
    /**
     * @brief Export project report as PDF
     */
    ExportResult exportProjectReport(const std::string& filePath,
                                   bool includeImages = true,
                                   bool includeBOM = true,
                                   bool includeSpecifications = true);
    
    /**
     * @brief Export technical drawings as PDF
     */
    ExportResult exportTechnicalDrawings(const std::string& filePath,
                                       bool includeTopView = true,
                                       bool includeFrontView = true,
                                       bool includeSideView = true,
                                       bool includeIsometric = true);
    
    // Export validation and preview
    
    /**
     * @brief Validate export parameters
     */
    bool validateExportPath(const std::string& filePath, CADFormat format) const;
    
    /**
     * @brief Check if objects can be exported in format
     */
    bool canExportObjects(const std::vector<std::string>& objectIds, CADFormat format) const;
    
    /**
     * @brief Estimate export file size
     */
    size_t estimateFileSize(const std::vector<std::string>& objectIds, 
                           const ExportOptions& options) const;
    
    /**
     * @brief Estimate export time
     */
    double estimateExportTime(const std::vector<std::string>& objectIds, 
                            const ExportOptions& options) const;
    
    // Export settings management
    
    /**
     * @brief Set default export options
     */
    void setDefaultExportOptions(const ExportOptions& options);
    
    /**
     * @brief Get default export options
     */
    ExportOptions getDefaultExportOptions() const;
    
    /**
     * @brief Create export options with common settings
     */
    ExportOptions createExportOptions(CADFormat format, 
                                    ExportQuality quality = ExportQuality::Standard,
                                    bool includeTextures = true,
                                    bool includeMaterials = true,
                                    bool includeMetadata = true) const;
    
    // File and path utilities
    
    /**
     * @brief Suggest filename for export
     */
    std::string suggestFileName(CADFormat format, const std::string& baseName = "") const;
    
    /**
     * @brief Validate filename for format
     */
    bool isValidFileName(const std::string& fileName, CADFormat format) const;
    
    /**
     * @brief Generate unique filename
     */
    std::string generateUniqueFileName(const std::string& baseFilePath) const;
    
    /**
     * @brief Get export directory from project
     */
    std::string getDefaultExportDirectory() const;
    
    /**
     * @brief Create export directory if it doesn't exist
     */
    bool createExportDirectory(const std::string& directory) const;
    
    // Progress and status
    
    /**
     * @brief Set progress callback
     */
    void setProgressCallback(std::function<void(double, const std::string&)> callback) {
        progressCallback_ = callback;
    }
    
    /**
     * @brief Set export completed callback
     */
    void setExportCompletedCallback(std::function<void(const ExportResult&)> callback) {
        exportCompletedCallback_ = callback;
    }
    
    /**
     * @brief Set error callback
     */
    void setErrorCallback(std::function<void(const std::string&)> callback) {
        errorCallback_ = callback;
    }
    
    /**
     * @brief Clear all callbacks
     */
    void clearCallbacks();
    
    // Error handling
    
    /**
     * @brief Get last export error
     */
    std::string getLastError() const;
    
    /**
     * @brief Clear last error
     */
    void clearLastError();
    
    // Export history and statistics
    
    /**
     * @brief Export history entry
     */
    struct ExportHistoryEntry {
        std::string filePath;
        CADFormat format;
        ExportQuality quality;
        std::chrono::system_clock::time_point timestamp;
        bool success;
        size_t fileSize;
        double exportTime;
        std::string errorMessage;
    };
    
    /**
     * @brief Get export history
     */
    std::vector<ExportHistoryEntry> getExportHistory(size_t maxEntries = 50) const;
    
    /**
     * @brief Clear export history
     */
    void clearExportHistory();
    
    /**
     * @brief Get export statistics
     */
    struct ExportStatistics {
        size_t totalExports = 0;
        size_t successfulExports = 0;
        size_t failedExports = 0;
        double averageExportTime = 0.0;
        size_t totalBytesExported = 0;
        std::unordered_map<CADFormat, size_t> formatCounts;
    };
    ExportStatistics getExportStatistics() const;

private:
    std::vector<ExportHistoryEntry> exportHistory_;
    std::string lastError_;
    
    /**
     * @brief Get objects from project by IDs
     */
    std::vector<const SceneObject*> getObjectsById(const std::vector<std::string>& objectIds) const;
    
    /**
     * @brief Get objects by category
     */
    std::vector<const SceneObject*> getObjectsByCategory(const std::string& category) const;
    
    /**
     * @brief Get objects in region
     */
    std::vector<const SceneObject*> getObjectsInRegion(const BoundingBox& region) const;
    
    /**
     * @brief Record export in history
     */
    void recordExport(const ExportResult& result, CADFormat format, ExportQuality quality);
    
    /**
     * @brief Notify callbacks
     */
    void notifyProgress(double progress, const std::string& status);
    void notifyExportCompleted(const ExportResult& result);
    void notifyError(const std::string& error);
    
    /**
     * @brief Validate export preconditions
     */
    bool validateExportPreconditions(const std::string& filePath) const;
    
    /**
     * @brief Setup CAD exporter callbacks
     */
    void setupCADExporterCallbacks();
};

} // namespace Controllers
} // namespace KitchenCAD