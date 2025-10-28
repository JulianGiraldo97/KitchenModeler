#pragma once

#include <string>
#include <vector>
#include <memory>

namespace KitchenCAD {

// Forward declarations
class Project;
class SceneObject;

/**
 * @brief Supported CAD export formats
 */
enum class CADFormat {
    STEP,       // Standard for the Exchange of Product Data
    IGES,       // Initial Graphics Exchange Specification
    STL,        // Stereolithography format
    OBJ,        // Wavefront OBJ format
    PLY,        // Polygon File Format
    COLLADA,    // COLLAborative Design Activity format
    GLTF,       // GL Transmission Format
    X3D         // Extensible 3D format
};

/**
 * @brief Export quality settings
 */
enum class ExportQuality {
    Draft,      // Low resolution, fast export
    Standard,   // Balanced quality/size
    High,       // High resolution
    Production  // Maximum quality
};

/**
 * @brief Export options configuration
 */
struct ExportOptions {
    CADFormat format = CADFormat::STEP;
    ExportQuality quality = ExportQuality::Standard;
    bool includeTextures = true;
    bool includeMaterials = true;
    bool includeMetadata = true;
    bool mergeObjects = false;
    double tolerance = 0.01;  // Geometric tolerance in mm
    double angularTolerance = 0.1;  // Angular tolerance in radians
    std::string units = "mm";  // Units: mm, cm, m, in, ft
    
    ExportOptions() = default;
    ExportOptions(CADFormat fmt) : format(fmt) {}
};

/**
 * @brief Export result information
 */
struct ExportResult {
    bool success = false;
    std::string message;
    std::string filePath;
    size_t fileSize = 0;  // Size in bytes
    double exportTime = 0.0;  // Time in seconds
    size_t objectCount = 0;
    size_t triangleCount = 0;
    
    ExportResult() = default;
    ExportResult(bool success, const std::string& message) 
        : success(success), message(message) {}
};

/**
 * @brief Interface for CAD export operations
 * 
 * This interface defines the contract for exporting 3D models and scenes
 * to various CAD formats for interoperability with other software.
 */
class ICADExporter {
public:
    virtual ~ICADExporter() = default;
    
    // Format support queries
    virtual std::vector<CADFormat> getSupportedFormats() const = 0;
    virtual bool isFormatSupported(CADFormat format) const = 0;
    virtual std::string getFormatExtension(CADFormat format) const = 0;
    virtual std::string getFormatDescription(CADFormat format) const = 0;
    
    // Single object export
    virtual ExportResult exportObject(const SceneObject& object,
                                    const std::string& filePath,
                                    const ExportOptions& options = ExportOptions()) = 0;
    
    // Multiple objects export
    virtual ExportResult exportObjects(const std::vector<const SceneObject*>& objects,
                                     const std::string& filePath,
                                     const ExportOptions& options = ExportOptions()) = 0;
    
    // Full project export
    virtual ExportResult exportProject(const Project& project,
                                     const std::string& filePath,
                                     const ExportOptions& options = ExportOptions()) = 0;
    
    // Selective export by category
    virtual ExportResult exportByCategory(const Project& project,
                                        const std::string& category,
                                        const std::string& filePath,
                                        const ExportOptions& options = ExportOptions()) = 0;
    
    // Selective export by selection
    virtual ExportResult exportSelection(const Project& project,
                                       const std::vector<std::string>& objectIds,
                                       const std::string& filePath,
                                       const ExportOptions& options = ExportOptions()) = 0;
    
    // Batch export operations
    virtual std::vector<ExportResult> exportMultipleFormats(const Project& project,
                                                           const std::string& baseFilePath,
                                                           const std::vector<CADFormat>& formats,
                                                           const ExportOptions& options = ExportOptions()) = 0;
    
    // Export validation
    virtual bool validateExportPath(const std::string& filePath, CADFormat format) = 0;
    virtual bool canExportObject(const SceneObject& object, CADFormat format) = 0;
    
    // Export preview/analysis
    virtual size_t estimateFileSize(const Project& project, const ExportOptions& options) = 0;
    virtual double estimateExportTime(const Project& project, const ExportOptions& options) = 0;
    
    // Progress callback support
    using ProgressCallback = std::function<void(double progress, const std::string& status)>;
    virtual void setProgressCallback(ProgressCallback callback) = 0;
    virtual void clearProgressCallback() = 0;
    
    // Export settings management
    virtual void setDefaultOptions(const ExportOptions& options) = 0;
    virtual const ExportOptions& getDefaultOptions() const = 0;
    
    // Utility functions
    virtual std::string suggestFileName(const Project& project, CADFormat format) = 0;
    virtual bool isValidFileName(const std::string& fileName, CADFormat format) = 0;
    
    // Error handling
    virtual std::string getLastError() const = 0;
    virtual void clearLastError() = 0;
};

} // namespace KitchenCAD