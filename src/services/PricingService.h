#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <chrono>
#include "../models/Project.h"
#include "../models/CatalogItem.h"

namespace KitchenCAD {
namespace Services {

using namespace Models;

/**
 * @brief Price calculation result
 */
struct PriceCalculation {
    double basePrice = 0.0;
    double materialCost = 0.0;
    double laborCost = 0.0;
    double hardwareCost = 0.0;
    double overheadCost = 0.0;
    double discountAmount = 0.0;
    double taxAmount = 0.0;
    double totalPrice = 0.0;
    
    std::string currency = "USD";
    std::chrono::system_clock::time_point calculatedAt;
    
    // Breakdown by category
    std::unordered_map<std::string, double> categoryBreakdown;
    
    // Notes and details
    std::vector<std::string> notes;
    std::vector<std::string> warnings;
    
    PriceCalculation() {
        calculatedAt = std::chrono::system_clock::now();
    }
    
    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& json);
};

/**
 * @brief Pricing rule for dynamic pricing
 */
struct PricingRule {
    std::string id;
    std::string name;
    std::string description;
    bool enabled = true;
    
    // Conditions
    std::string category; // Empty means all categories
    double minQuantity = 0.0;
    double maxQuantity = std::numeric_limits<double>::max();
    double minValue = 0.0;
    double maxValue = std::numeric_limits<double>::max();
    
    // Pricing adjustments
    enum class AdjustmentType {
        Percentage,
        FixedAmount,
        Multiplier
    } adjustmentType = AdjustmentType::Percentage;
    
    double adjustmentValue = 0.0;
    
    // Validity period
    std::chrono::system_clock::time_point validFrom;
    std::chrono::system_clock::time_point validTo;
    
    bool isValid(const std::chrono::system_clock::time_point& when = std::chrono::system_clock::now()) const;
    bool appliesTo(const std::string& itemCategory, double quantity, double value) const;
    double applyAdjustment(double originalPrice) const;
    
    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& json);
};

/**
 * @brief Pricing configuration
 */
struct PricingConfiguration {
    // Base rates
    double laborRatePerHour = 50.0;
    double overheadPercentage = 0.15; // 15%
    double taxRate = 0.08; // 8%
    double profitMargin = 0.25; // 25%
    
    // Currency and formatting
    std::string currency = "USD";
    int decimalPlaces = 2;
    std::string currencySymbol = "$";
    
    // Discounts
    double volumeDiscountThreshold = 10000.0; // Minimum order value for volume discount
    double volumeDiscountRate = 0.05; // 5% volume discount
    double loyaltyDiscountRate = 0.03; // 3% loyalty discount
    
    // Material pricing
    std::unordered_map<std::string, double> materialPrices; // per m²
    std::unordered_map<std::string, double> hardwarePrices; // per unit
    std::unordered_map<std::string, double> laborMultipliers; // by category
    
    // Pricing rules
    std::vector<PricingRule> rules;
    
    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& json);
};

/**
 * @brief Pricing service for cost calculations
 * 
 * This service handles all pricing calculations including materials, labor,
 * overhead, taxes, discounts, and dynamic pricing rules.
 * Implements requirements 8.1, 8.2, 8.5, 15.1, 15.2, 15.3, 15.4
 */
class PricingService {
private:
    PricingConfiguration config_;
    
    // Price history for tracking
    std::unordered_map<std::string, std::vector<PriceCalculation>> priceHistory_;
    
    // Callbacks
    std::function<void(const std::string&)> priceUpdatedCallback_;
    std::function<void(const std::string&)> errorCallback_;

public:
    /**
     * @brief Constructor
     */
    explicit PricingService(const PricingConfiguration& config = PricingConfiguration{});
    
    /**
     * @brief Destructor
     */
    ~PricingService() = default;
    
    // Price calculations
    
    /**
     * @brief Calculate price for single catalog item
     */
    PriceCalculation calculateItemPrice(const CatalogItem& item,
                                       const MaterialProperties& material = MaterialProperties{},
                                       int quantity = 1) const;
    
    /**
     * @brief Calculate price for scene object
     */
    PriceCalculation calculateObjectPrice(const SceneObject& object,
                                         const CatalogItem& catalogItem) const;
    
    /**
     * @brief Calculate total project price
     */
    PriceCalculation calculateProjectPrice(const Project& project,
                                          const std::function<std::shared_ptr<CatalogItem>(const std::string&)>& catalogLookup) const;
    
    /**
     * @brief Calculate price for list of items
     */
    PriceCalculation calculateBulkPrice(const std::vector<std::pair<std::shared_ptr<CatalogItem>, int>>& items) const;
    
    /**
     * @brief Calculate material cost for item
     */
    double calculateMaterialCost(const CatalogItem& item, const MaterialProperties& material) const;
    
    /**
     * @brief Calculate labor cost for item
     */
    double calculateLaborCost(const CatalogItem& item, int quantity = 1) const;
    
    /**
     * @brief Calculate hardware cost for item
     */
    double calculateHardwareCost(const CatalogItem& item, int quantity = 1) const;
    
    // Pricing rules management
    
    /**
     * @brief Add pricing rule
     */
    void addPricingRule(const PricingRule& rule);
    
    /**
     * @brief Remove pricing rule
     */
    bool removePricingRule(const std::string& ruleId);
    
    /**
     * @brief Update pricing rule
     */
    bool updatePricingRule(const PricingRule& rule);
    
    /**
     * @brief Get pricing rule by ID
     */
    const PricingRule* getPricingRule(const std::string& ruleId) const;
    
    /**
     * @brief Get all pricing rules
     */
    const std::vector<PricingRule>& getPricingRules() const { return config_.rules; }
    
    /**
     * @brief Apply pricing rules to calculation
     */
    void applyPricingRules(PriceCalculation& calculation, const std::string& category, 
                          double quantity, double baseValue) const;
    
    // Configuration management
    
    /**
     * @brief Set pricing configuration
     */
    void setConfiguration(const PricingConfiguration& config) { config_ = config; }
    
    /**
     * @brief Get pricing configuration
     */
    const PricingConfiguration& getConfiguration() const { return config_; }
    
    /**
     * @brief Update material price
     */
    void setMaterialPrice(const std::string& materialId, double pricePerSquareMeter);
    
    /**
     * @brief Get material price
     */
    double getMaterialPrice(const std::string& materialId) const;
    
    /**
     * @brief Update hardware price
     */
    void setHardwarePrice(const std::string& hardwareId, double pricePerUnit);
    
    /**
     * @brief Get hardware price
     */
    double getHardwarePrice(const std::string& hardwareId) const;
    
    /**
     * @brief Set labor rate
     */
    void setLaborRate(double ratePerHour) { config_.laborRatePerHour = ratePerHour; }
    
    /**
     * @brief Get labor rate
     */
    double getLaborRate() const { return config_.laborRatePerHour; }
    
    /**
     * @brief Set tax rate
     */
    void setTaxRate(double rate) { config_.taxRate = rate; }
    
    /**
     * @brief Get tax rate
     */
    double getTaxRate() const { return config_.taxRate; }
    
    /**
     * @brief Set currency
     */
    void setCurrency(const std::string& currency, const std::string& symbol = "");
    
    /**
     * @brief Get currency
     */
    const std::string& getCurrency() const { return config_.currency; }
    
    // Discounts and adjustments
    
    /**
     * @brief Calculate volume discount
     */
    double calculateVolumeDiscount(double totalValue) const;
    
    /**
     * @brief Calculate loyalty discount
     */
    double calculateLoyaltyDiscount(double totalValue, bool isLoyalCustomer = false) const;
    
    /**
     * @brief Apply custom discount
     */
    PriceCalculation applyDiscount(const PriceCalculation& original, double discountPercentage, 
                                  const std::string& reason = "") const;
    
    /**
     * @brief Apply markup
     */
    PriceCalculation applyMarkup(const PriceCalculation& original, double markupPercentage,
                                const std::string& reason = "") const;
    
    // Price history and tracking
    
    /**
     * @brief Record price calculation
     */
    void recordPriceCalculation(const std::string& itemId, const PriceCalculation& calculation);
    
    /**
     * @brief Get price history for item
     */
    std::vector<PriceCalculation> getPriceHistory(const std::string& itemId) const;
    
    /**
     * @brief Get latest price for item
     */
    std::optional<PriceCalculation> getLatestPrice(const std::string& itemId) const;
    
    /**
     * @brief Clear price history
     */
    void clearPriceHistory(const std::string& itemId = "");
    
    // Import/Export
    
    /**
     * @brief Load configuration from file
     */
    bool loadConfigurationFromFile(const std::string& filename);
    
    /**
     * @brief Save configuration to file
     */
    bool saveConfigurationToFile(const std::string& filename) const;
    
    /**
     * @brief Import pricing data from CSV
     */
    bool importPricingFromCSV(const std::string& filename);
    
    /**
     * @brief Export pricing data to CSV
     */
    bool exportPricingToCSV(const std::string& filename) const;
    
    // Validation and analysis
    
    /**
     * @brief Validate pricing configuration
     */
    std::vector<std::string> validateConfiguration() const;
    
    /**
     * @brief Get pricing statistics
     */
    struct PricingStatistics {
        double averageItemPrice = 0.0;
        double minItemPrice = 0.0;
        double maxItemPrice = 0.0;
        std::unordered_map<std::string, double> averagePriceByCategory;
        size_t totalCalculations = 0;
        std::chrono::system_clock::time_point lastCalculation;
    };
    
    PricingStatistics getPricingStatistics() const;
    
    /**
     * @brief Compare prices with previous calculations
     */
    struct PriceComparison {
        double currentPrice = 0.0;
        double previousPrice = 0.0;
        double changeAmount = 0.0;
        double changePercentage = 0.0;
        std::string trend; // "up", "down", "stable"
    };
    
    PriceComparison comparePrices(const std::string& itemId, const PriceCalculation& current) const;
    
    // Callbacks
    
    /**
     * @brief Set callback for price updates
     */
    void setPriceUpdatedCallback(std::function<void(const std::string&)> callback) {
        priceUpdatedCallback_ = callback;
    }
    
    /**
     * @brief Set callback for errors
     */
    void setErrorCallback(std::function<void(const std::string&)> callback) {
        errorCallback_ = callback;
    }
    
    // Utility functions
    
    /**
     * @brief Format price as string
     */
    std::string formatPrice(double price) const;
    
    /**
     * @brief Parse price from string
     */
    double parsePrice(const std::string& priceString) const;
    
    /**
     * @brief Convert currency (placeholder for future implementation)
     */
    double convertCurrency(double amount, const std::string& fromCurrency, 
                          const std::string& toCurrency) const;
    
    /**
     * @brief Estimate item complexity for labor calculation
     */
    static double estimateComplexity(const CatalogItem& item);
    
    /**
     * @brief Calculate surface area for material pricing
     */
    static double calculateSurfaceArea(const CatalogItem& item);

private:
    /**
     * @brief Apply overhead and profit margin
     */
    void applyOverheadAndProfit(PriceCalculation& calculation) const;
    
    /**
     * @brief Calculate taxes
     */
    void calculateTaxes(PriceCalculation& calculation) const;
    
    /**
     * @brief Notify price updated callback
     */
    void notifyPriceUpdated(const std::string& message);
    
    /**
     * @brief Notify error callback
     */
    void notifyError(const std::string& error) const;
    
    /**
     * @brief Initialize default pricing data
     */
    void initializeDefaultPricing();
};

} // namespace Services
} // namespace KitchenCAD