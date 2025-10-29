#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include "../models/Project.h"
#include "../models/CatalogItem.h"

namespace KitchenCAD {
namespace Services {

using namespace Models;

/**
 * @brief Generation options for BOM
 */
struct GenerationOptions {
    bool includeMaterials = true;
    bool includeHardware = true;
    bool includeLabor = false;
    bool optimizeCuts = true;
    bool groupSimilarItems = true;
    double laborRatePerHour = 50.0;
    double overheadPercentage = 0.15; // 15%
    double taxRate = 0.08; // 8%
    std::string currency = "USD";
    
    // Cut optimization settings
    struct CutSettings {
        double sawKerf = 3.0; // mm
        double minCutSize = 100.0; // mm
        double sheetWidth = 2440.0; // mm (standard 8ft)
        double sheetHeight = 1220.0; // mm (standard 4ft)
        std::vector<double> standardThicknesses = {18.0, 25.0, 32.0}; // mm
    } cutSettings;
};

/**
 * @brief Bill of Materials (BOM) item
 */
struct BOMItem {
    std::string itemId;
    std::string catalogItemId;
    std::string name;
    std::string category;
    int quantity;
    Dimensions3D dimensions;
    std::string materialId;
    std::string materialName;
    double unitPrice;
    double materialCost;
    double totalPrice;
    std::string supplier;
    std::string supplierCode;
    std::string notes;
    
    // Cut optimization data
    struct CutInfo {
        double length = 0.0;
        double width = 0.0;
        double thickness = 0.0;
        std::string material;
        int sheetCount = 0;
        double wastePercentage = 0.0;
    } cutInfo;
    
    BOMItem() : quantity(0), unitPrice(0.0), materialCost(0.0), totalPrice(0.0) {}
    
    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& json);
};

/**
 * @brief Complete Bill of Materials
 */
class BillOfMaterials {
private:
    std::vector<BOMItem> items_;
    double totalCost_;
    double materialCost_;
    double laborCost_;
    double overheadCost_;
    double taxRate_;
    std::string currency_;
    std::chrono::system_clock::time_point generatedAt_;
    
    // Metadata
    std::string projectId_;
    std::string projectName_;
    std::string customerName_;
    std::string notes_;

public:
    BillOfMaterials() : totalCost_(0.0), materialCost_(0.0), laborCost_(0.0), 
                       overheadCost_(0.0), taxRate_(0.0), currency_("USD") {
        generatedAt_ = std::chrono::system_clock::now();
    }
    
    // Item management
    void addItem(const BOMItem& item);
    void removeItem(const std::string& itemId);
    void updateItem(const BOMItem& item);
    BOMItem* getItem(const std::string& itemId);
    const BOMItem* getItem(const std::string& itemId) const;
    
    const std::vector<BOMItem>& getItems() const { return items_; }
    size_t getItemCount() const { return items_.size(); }
    void clear() { items_.clear(); recalculateTotals(); }
    
    // Cost calculations
    double getTotalCost() const { return totalCost_; }
    double getMaterialCost() const { return materialCost_; }
    double getLaborCost() const { return laborCost_; }
    double getOverheadCost() const { return overheadCost_; }
    double getTaxAmount() const { return (materialCost_ + laborCost_ + overheadCost_) * taxRate_; }
    double getGrandTotal() const { return totalCost_ + getTaxAmount(); }
    
    void setLaborCost(double cost) { laborCost_ = cost; recalculateTotals(); }
    void setOverheadCost(double cost) { overheadCost_ = cost; recalculateTotals(); }
    void setTaxRate(double rate) { taxRate_ = rate; }
    
    // Metadata
    const std::string& getProjectId() const { return projectId_; }
    void setProjectId(const std::string& id) { projectId_ = id; }
    
    const std::string& getProjectName() const { return projectName_; }
    void setProjectName(const std::string& name) { projectName_ = name; }
    
    const std::string& getCustomerName() const { return customerName_; }
    void setCustomerName(const std::string& name) { customerName_ = name; }
    
    const std::string& getCurrency() const { return currency_; }
    void setCurrency(const std::string& currency) { currency_ = currency; }
    
    const std::string& getNotes() const { return notes_; }
    void setNotes(const std::string& notes) { notes_ = notes; }
    
    std::chrono::system_clock::time_point getGeneratedAt() const { return generatedAt_; }
    
    // Analysis
    std::unordered_map<std::string, double> getCostsByCategory() const;
    std::unordered_map<std::string, int> getQuantitiesByCategory() const;
    std::vector<BOMItem> getItemsByCategory(const std::string& category) const;
    std::vector<BOMItem> getMostExpensiveItems(size_t count = 10) const;
    
    // Export functions
    bool exportToCSV(const std::string& filename) const;
    bool exportToJSON(const std::string& filename) const;
    bool exportToPDF(const std::string& filename) const; // TODO: Implement with PDF library
    std::string generateReport() const;
    
    // Serialization
    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& json);
    
    void recalculateTotals();
    
    // Allow BOMGenerator to access private members
    friend class BOMGenerator;
    
private:
};

/**
 * @brief Cut optimization result
 */
struct CutOptimization {
    struct Sheet {
        double width;
        double height;
        double thickness;
        std::string material;
        std::vector<BOMItem*> items;
        double utilization; // Percentage of sheet used
    };
    
    std::vector<Sheet> sheets;
    double totalWaste;
    double totalCost;
    int totalSheets;
    
    nlohmann::json toJson() const;
};

/**
 * @brief BOM Generator service
 * 
 * This service generates bills of materials from projects, optimizes cuts,
 * calculates costs, and provides various export formats.
 * Implements requirements 8.1, 8.2, 8.3, 8.4, 8.5
 */
class BOMGenerator {
public:
    
    /**
     * @brief Pricing configuration
     */
    struct PricingConfig {
        std::unordered_map<std::string, double> materialPrices; // per square meter
        std::unordered_map<std::string, double> hardwarePrices; // per unit
        std::unordered_map<std::string, double> laborTimes; // hours per item
        double defaultLaborRate = 50.0;
        std::string defaultCurrency = "USD";
    };

private:
    GenerationOptions defaultOptions_;
    PricingConfig pricingConfig_;
    
    // Callbacks
    std::function<void(const std::string&)> progressCallback_;
    std::function<void(const std::string&)> errorCallback_;

public:
    /**
     * @brief Constructor
     */
    explicit BOMGenerator(const GenerationOptions& options = GenerationOptions());
    
    /**
     * @brief Generate BOM from project
     */
    std::unique_ptr<BillOfMaterials> generateBOM(const Project& project,
                                                const std::function<std::shared_ptr<CatalogItem>(const std::string&)>& catalogLookup,
                                                const GenerationOptions& options = GenerationOptions());
    
    /**
     * @brief Generate BOM from selected objects
     */
    std::unique_ptr<BillOfMaterials> generateBOMFromObjects(const std::vector<const SceneObject*>& objects,
                                                           const std::function<std::shared_ptr<CatalogItem>(const std::string&)>& catalogLookup,
                                                           const GenerationOptions& options = GenerationOptions());
    
    /**
     * @brief Optimize cuts for BOM
     */
    CutOptimization optimizeCuts(BillOfMaterials& bom, const GenerationOptions::CutSettings& settings = GenerationOptions::CutSettings());
    
    /**
     * @brief Calculate labor costs
     */
    double calculateLaborCost(const BillOfMaterials& bom, const GenerationOptions& options = GenerationOptions());
    
    /**
     * @brief Update BOM with current pricing
     */
    void updatePricing(BillOfMaterials& bom);
    
    /**
     * @brief Validate BOM completeness
     */
    std::vector<std::string> validateBOM(const BillOfMaterials& bom) const;
    
    // Configuration
    
    /**
     * @brief Set default generation options
     */
    void setDefaultOptions(const GenerationOptions& options) { defaultOptions_ = options; }
    
    /**
     * @brief Get default generation options
     */
    const GenerationOptions& getDefaultOptions() const { return defaultOptions_; }
    
    /**
     * @brief Set pricing configuration
     */
    void setPricingConfig(const PricingConfig& config) { pricingConfig_ = config; }
    
    /**
     * @brief Get pricing configuration
     */
    const PricingConfig& getPricingConfig() const { return pricingConfig_; }
    
    /**
     * @brief Load pricing from file
     */
    bool loadPricingFromFile(const std::string& filename);
    
    /**
     * @brief Save pricing to file
     */
    bool savePricingToFile(const std::string& filename) const;
    
    // Callbacks
    
    /**
     * @brief Set progress callback
     */
    void setProgressCallback(std::function<void(const std::string&)> callback) {
        progressCallback_ = callback;
    }
    
    /**
     * @brief Set error callback
     */
    void setErrorCallback(std::function<void(const std::string&)> callback) {
        errorCallback_ = callback;
    }
    
    // Utility functions
    
    /**
     * @brief Calculate material area for item
     */
    static double calculateMaterialArea(const CatalogItem& item, const MaterialProperties& material);
    
    /**
     * @brief Calculate hardware count for item
     */
    static int calculateHardwareCount(const CatalogItem& item);
    
    /**
     * @brief Estimate labor time for item
     */
    static double estimateLaborTime(const CatalogItem& item);
    
    /**
     * @brief Group similar BOM items
     */
    void groupSimilarItems(BillOfMaterials& bom) const;
    
    /**
     * @brief Generate item description
     */
    static std::string generateItemDescription(const CatalogItem& item, const MaterialProperties& material);

private:
    /**
     * @brief Process single scene object
     */
    BOMItem processSceneObject(const SceneObject& object,
                              const std::shared_ptr<CatalogItem>& catalogItem,
                              const GenerationOptions& options);
    
    /**
     * @brief Calculate item pricing
     */
    void calculateItemPricing(BOMItem& item, const GenerationOptions& options);
    
    /**
     * @brief Optimize cuts using bin packing algorithm
     */
    std::vector<CutOptimization::Sheet> optimizeCutLayout(const std::vector<BOMItem*>& items,
                                                         const GenerationOptions::CutSettings& settings);
    
    /**
     * @brief Notify progress
     */
    void notifyProgress(const std::string& message);
    
    /**
     * @brief Notify error
     */
    void notifyError(const std::string& error);
    
    /**
     * @brief Initialize default pricing
     */
    void initializeDefaultPricing();
};

} // namespace Services
} // namespace KitchenCAD