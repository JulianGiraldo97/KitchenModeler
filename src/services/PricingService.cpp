#include "PricingService.h"
#include "../utils/Logger.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

namespace KitchenCAD {
namespace Services {

// PriceCalculation implementation
nlohmann::json PriceCalculation::toJson() const {
    nlohmann::json json;
    json["basePrice"] = basePrice;
    json["materialCost"] = materialCost;
    json["laborCost"] = laborCost;
    json["hardwareCost"] = hardwareCost;
    json["overheadCost"] = overheadCost;
    json["discountAmount"] = discountAmount;
    json["taxAmount"] = taxAmount;
    json["totalPrice"] = totalPrice;
    json["currency"] = currency;
    json["calculatedAt"] = calculatedAt.time_since_epoch().count();
    
    nlohmann::json categoryJson;
    for (const auto& pair : categoryBreakdown) {
        categoryJson[pair.first] = pair.second;
    }
    json["categoryBreakdown"] = categoryJson;
    
    json["notes"] = notes;
    json["warnings"] = warnings;
    
    return json;
}

void PriceCalculation::fromJson(const nlohmann::json& json) {
    basePrice = json.value("basePrice", 0.0);
    materialCost = json.value("materialCost", 0.0);
    laborCost = json.value("laborCost", 0.0);
    hardwareCost = json.value("hardwareCost", 0.0);
    overheadCost = json.value("overheadCost", 0.0);
    discountAmount = json.value("discountAmount", 0.0);
    taxAmount = json.value("taxAmount", 0.0);
    totalPrice = json.value("totalPrice", 0.0);
    currency = json.value("currency", "USD");
    
    if (json.contains("calculatedAt")) {
        auto timePoint = std::chrono::system_clock::time_point(
            std::chrono::system_clock::duration(json["calculatedAt"].get<long long>()));
        calculatedAt = timePoint;
    }
    
    categoryBreakdown.clear();
    if (json.contains("categoryBreakdown")) {
        for (const auto& item : json["categoryBreakdown"].items()) {
            categoryBreakdown[item.key()] = item.value().get<double>();
        }
    }
    
    notes.clear();
    if (json.contains("notes")) {
        for (const auto& note : json["notes"]) {
            notes.push_back(note.get<std::string>());
        }
    }
    
    warnings.clear();
    if (json.contains("warnings")) {
        for (const auto& warning : json["warnings"]) {
            warnings.push_back(warning.get<std::string>());
        }
    }
}

// PricingRule implementation
bool PricingRule::isValid(const std::chrono::system_clock::time_point& when) const {
    if (!enabled) return false;
    
    return when >= validFrom && when <= validTo;
}

bool PricingRule::appliesTo(const std::string& itemCategory, double quantity, double value) const {
    if (!category.empty() && category != itemCategory) {
        return false;
    }
    
    if (quantity < minQuantity || quantity > maxQuantity) {
        return false;
    }
    
    if (value < minValue || value > maxValue) {
        return false;
    }
    
    return true;
}

double PricingRule::applyAdjustment(double originalPrice) const {
    switch (adjustmentType) {
        case AdjustmentType::Percentage:
            return originalPrice * (1.0 + adjustmentValue / 100.0);
        case AdjustmentType::FixedAmount:
            return originalPrice + adjustmentValue;
        case AdjustmentType::Multiplier:
            return originalPrice * adjustmentValue;
        default:
            return originalPrice;
    }
}

nlohmann::json PricingRule::toJson() const {
    nlohmann::json json;
    json["id"] = id;
    json["name"] = name;
    json["description"] = description;
    json["enabled"] = enabled;
    json["category"] = category;
    json["minQuantity"] = minQuantity;
    json["maxQuantity"] = maxQuantity;
    json["minValue"] = minValue;
    json["maxValue"] = maxValue;
    json["adjustmentType"] = static_cast<int>(adjustmentType);
    json["adjustmentValue"] = adjustmentValue;
    json["validFrom"] = validFrom.time_since_epoch().count();
    json["validTo"] = validTo.time_since_epoch().count();
    return json;
}

void PricingRule::fromJson(const nlohmann::json& json) {
    id = json.value("id", "");
    name = json.value("name", "");
    description = json.value("description", "");
    enabled = json.value("enabled", true);
    category = json.value("category", "");
    minQuantity = json.value("minQuantity", 0.0);
    maxQuantity = json.value("maxQuantity", std::numeric_limits<double>::max());
    minValue = json.value("minValue", 0.0);
    maxValue = json.value("maxValue", std::numeric_limits<double>::max());
    adjustmentType = static_cast<AdjustmentType>(json.value("adjustmentType", 0));
    adjustmentValue = json.value("adjustmentValue", 0.0);
    
    if (json.contains("validFrom")) {
        validFrom = std::chrono::system_clock::time_point(
            std::chrono::system_clock::duration(json["validFrom"].get<long long>()));
    }
    if (json.contains("validTo")) {
        validTo = std::chrono::system_clock::time_point(
            std::chrono::system_clock::duration(json["validTo"].get<long long>()));
    }
}

// PricingConfiguration implementation
nlohmann::json PricingConfiguration::toJson() const {
    nlohmann::json json;
    json["laborRatePerHour"] = laborRatePerHour;
    json["overheadPercentage"] = overheadPercentage;
    json["taxRate"] = taxRate;
    json["profitMargin"] = profitMargin;
    json["currency"] = currency;
    json["decimalPlaces"] = decimalPlaces;
    json["currencySymbol"] = currencySymbol;
    json["volumeDiscountThreshold"] = volumeDiscountThreshold;
    json["volumeDiscountRate"] = volumeDiscountRate;
    json["loyaltyDiscountRate"] = loyaltyDiscountRate;
    
    nlohmann::json materialPricesJson;
    for (const auto& pair : materialPrices) {
        materialPricesJson[pair.first] = pair.second;
    }
    json["materialPrices"] = materialPricesJson;
    
    nlohmann::json hardwarePricesJson;
    for (const auto& pair : hardwarePrices) {
        hardwarePricesJson[pair.first] = pair.second;
    }
    json["hardwarePrices"] = hardwarePricesJson;
    
    nlohmann::json laborMultipliersJson;
    for (const auto& pair : laborMultipliers) {
        laborMultipliersJson[pair.first] = pair.second;
    }
    json["laborMultipliers"] = laborMultipliersJson;
    
    nlohmann::json rulesArray = nlohmann::json::array();
    for (const auto& rule : rules) {
        rulesArray.push_back(rule.toJson());
    }
    json["rules"] = rulesArray;
    
    return json;
}

void PricingConfiguration::fromJson(const nlohmann::json& json) {
    laborRatePerHour = json.value("laborRatePerHour", 50.0);
    overheadPercentage = json.value("overheadPercentage", 0.15);
    taxRate = json.value("taxRate", 0.08);
    profitMargin = json.value("profitMargin", 0.25);
    currency = json.value("currency", "USD");
    decimalPlaces = json.value("decimalPlaces", 2);
    currencySymbol = json.value("currencySymbol", "$");
    volumeDiscountThreshold = json.value("volumeDiscountThreshold", 10000.0);
    volumeDiscountRate = json.value("volumeDiscountRate", 0.05);
    loyaltyDiscountRate = json.value("loyaltyDiscountRate", 0.03);
    
    materialPrices.clear();
    if (json.contains("materialPrices")) {
        for (const auto& item : json["materialPrices"].items()) {
            materialPrices[item.key()] = item.value().get<double>();
        }
    }
    
    hardwarePrices.clear();
    if (json.contains("hardwarePrices")) {
        for (const auto& item : json["hardwarePrices"].items()) {
            hardwarePrices[item.key()] = item.value().get<double>();
        }
    }
    
    laborMultipliers.clear();
    if (json.contains("laborMultipliers")) {
        for (const auto& item : json["laborMultipliers"].items()) {
            laborMultipliers[item.key()] = item.value().get<double>();
        }
    }
    
    rules.clear();
    if (json.contains("rules")) {
        for (const auto& ruleJson : json["rules"]) {
            PricingRule rule;
            rule.fromJson(ruleJson);
            rules.push_back(rule);
        }
    }
}

// PricingService implementation
PricingService::PricingService(const PricingConfiguration& config)
    : config_(config)
{
    initializeDefaultPricing();
}

PriceCalculation PricingService::calculateItemPrice(const CatalogItem& item,
                                                   const MaterialProperties& material,
                                                   int quantity) const {
    PriceCalculation calculation;
    calculation.currency = config_.currency;
    
    // Base price from catalog
    calculation.basePrice = item.getBasePrice() * quantity;
    
    // Material cost
    calculation.materialCost = calculateMaterialCost(item, material) * quantity;
    
    // Labor cost
    calculation.laborCost = calculateLaborCost(item, quantity);
    
    // Hardware cost
    calculation.hardwareCost = calculateHardwareCost(item, quantity);
    
    // Apply overhead and profit
    applyOverheadAndProfit(calculation);
    
    // Apply pricing rules
    applyPricingRules(calculation, item.getCategory(), quantity, calculation.basePrice);
    
    // Calculate taxes
    calculateTaxes(calculation);
    
    // Final total
    calculation.totalPrice = calculation.basePrice + calculation.materialCost + 
                           calculation.laborCost + calculation.hardwareCost + 
                           calculation.overheadCost - calculation.discountAmount + 
                           calculation.taxAmount;
    
    // Category breakdown
    calculation.categoryBreakdown[item.getCategory()] = calculation.totalPrice;
    
    return calculation;
}

PriceCalculation PricingService::calculateObjectPrice(const SceneObject& object,
                                                     const CatalogItem& catalogItem) const {
    return calculateItemPrice(catalogItem, object.getMaterial(), 1);
}

PriceCalculation PricingService::calculateProjectPrice(const Project& project,
                                                      const std::function<std::shared_ptr<CatalogItem>(const std::string&)>& catalogLookup) const {
    PriceCalculation totalCalculation;
    totalCalculation.currency = config_.currency;
    
    std::unordered_map<std::string, int> itemCounts;
    std::unordered_map<std::string, std::shared_ptr<CatalogItem>> catalogItems;
    
    // Count items and group by catalog item
    for (const auto& object : project.getObjects()) {
        const std::string& catalogItemId = object->getCatalogItemId();
        itemCounts[catalogItemId]++;
        
        if (catalogItems.find(catalogItemId) == catalogItems.end()) {
            catalogItems[catalogItemId] = catalogLookup(catalogItemId);
        }
    }
    
    // Calculate price for each unique item
    for (const auto& pair : itemCounts) {
        const std::string& catalogItemId = pair.first;
        int quantity = pair.second;
        
        auto catalogItem = catalogItems[catalogItemId];
        if (!catalogItem) {
            totalCalculation.warnings.push_back("Catalog item not found: " + catalogItemId);
            continue;
        }
        
        // Use default material for project-level calculation
        MaterialProperties defaultMaterial;
        PriceCalculation itemCalc = calculateItemPrice(*catalogItem, defaultMaterial, quantity);
        
        // Accumulate costs
        totalCalculation.basePrice += itemCalc.basePrice;
        totalCalculation.materialCost += itemCalc.materialCost;
        totalCalculation.laborCost += itemCalc.laborCost;
        totalCalculation.hardwareCost += itemCalc.hardwareCost;
        totalCalculation.overheadCost += itemCalc.overheadCost;
        totalCalculation.discountAmount += itemCalc.discountAmount;
        totalCalculation.taxAmount += itemCalc.taxAmount;
        
        // Category breakdown
        const std::string& category = catalogItem->getCategory();
        totalCalculation.categoryBreakdown[category] += itemCalc.totalPrice;
    }
    
    // Apply project-level discounts
    double projectValue = totalCalculation.basePrice + totalCalculation.materialCost + 
                         totalCalculation.laborCost + totalCalculation.hardwareCost;
    
    double volumeDiscount = calculateVolumeDiscount(projectValue);
    totalCalculation.discountAmount += volumeDiscount;
    
    // Final total
    totalCalculation.totalPrice = totalCalculation.basePrice + totalCalculation.materialCost + 
                                totalCalculation.laborCost + totalCalculation.hardwareCost + 
                                totalCalculation.overheadCost - totalCalculation.discountAmount + 
                                totalCalculation.taxAmount;
    
    return totalCalculation;
}

double PricingService::calculateMaterialCost(const CatalogItem& item, const MaterialProperties& material) const {
    if (material.id.empty()) {
        return 0.0; // No material specified
    }
    
    auto it = config_.materialPrices.find(material.id);
    if (it == config_.materialPrices.end()) {
        return 0.0; // Material price not found
    }
    
    double pricePerSquareMeter = it->second;
    double surfaceArea = calculateSurfaceArea(item);
    
    return surfaceArea * pricePerSquareMeter;
}

double PricingService::calculateLaborCost(const CatalogItem& item, int quantity) const {
    double complexity = estimateComplexity(item);
    double baseHours = complexity * 0.5; // Base hours per complexity unit
    
    // Apply category multiplier
    auto it = config_.laborMultipliers.find(item.getCategory());
    if (it != config_.laborMultipliers.end()) {
        baseHours *= it->second;
    }
    
    return baseHours * quantity * config_.laborRatePerHour;
}

double PricingService::calculateHardwareCost(const CatalogItem& item, int quantity) const {
    double hardwareCost = 0.0;
    
    // Estimate hardware based on category
    std::string category = item.getCategory();
    
    if (category.find("cabinet") != std::string::npos) {
        // Cabinets need hinges and handles
        auto hingeIt = config_.hardwarePrices.find("hinge");
        auto handleIt = config_.hardwarePrices.find("handle");
        
        if (hingeIt != config_.hardwarePrices.end()) {
            hardwareCost += hingeIt->second * 2; // 2 hinges per door
        }
        if (handleIt != config_.hardwarePrices.end()) {
            hardwareCost += handleIt->second * 1; // 1 handle per door
        }
    } else if (category.find("drawer") != std::string::npos) {
        // Drawers need slides
        auto slideIt = config_.hardwarePrices.find("slide");
        if (slideIt != config_.hardwarePrices.end()) {
            hardwareCost += slideIt->second * 2; // 2 slides per drawer
        }
    }
    
    return hardwareCost * quantity;
}

void PricingService::addPricingRule(const PricingRule& rule) {
    // Remove existing rule with same ID
    removePricingRule(rule.id);
    config_.rules.push_back(rule);
}

bool PricingService::removePricingRule(const std::string& ruleId) {
    auto it = std::find_if(config_.rules.begin(), config_.rules.end(),
        [&ruleId](const PricingRule& rule) { return rule.id == ruleId; });
    
    if (it != config_.rules.end()) {
        config_.rules.erase(it);
        return true;
    }
    return false;
}

bool PricingService::updatePricingRule(const PricingRule& rule) {
    auto it = std::find_if(config_.rules.begin(), config_.rules.end(),
        [&rule](const PricingRule& existing) { return existing.id == rule.id; });
    
    if (it != config_.rules.end()) {
        *it = rule;
        return true;
    }
    return false;
}

const PricingRule* PricingService::getPricingRule(const std::string& ruleId) const {
    auto it = std::find_if(config_.rules.begin(), config_.rules.end(),
        [&ruleId](const PricingRule& rule) { return rule.id == ruleId; });
    
    return (it != config_.rules.end()) ? &(*it) : nullptr;
}

void PricingService::applyPricingRules(PriceCalculation& calculation, const std::string& category, 
                                      double quantity, double baseValue) const {
    for (const auto& rule : config_.rules) {
        if (rule.isValid() && rule.appliesTo(category, quantity, baseValue)) {
            double originalTotal = calculation.basePrice + calculation.materialCost + 
                                 calculation.laborCost + calculation.hardwareCost;
            double adjustedTotal = rule.applyAdjustment(originalTotal);
            double adjustment = adjustedTotal - originalTotal;
            
            if (adjustment < 0) {
                calculation.discountAmount += std::abs(adjustment);
                calculation.notes.push_back("Applied rule: " + rule.name + " (discount)");
            } else {
                calculation.overheadCost += adjustment;
                calculation.notes.push_back("Applied rule: " + rule.name + " (surcharge)");
            }
        }
    }
}

void PricingService::setMaterialPrice(const std::string& materialId, double pricePerSquareMeter) {
    config_.materialPrices[materialId] = pricePerSquareMeter;
    notifyPriceUpdated("Material price updated: " + materialId);
}

double PricingService::getMaterialPrice(const std::string& materialId) const {
    auto it = config_.materialPrices.find(materialId);
    return (it != config_.materialPrices.end()) ? it->second : 0.0;
}

void PricingService::setHardwarePrice(const std::string& hardwareId, double pricePerUnit) {
    config_.hardwarePrices[hardwareId] = pricePerUnit;
    notifyPriceUpdated("Hardware price updated: " + hardwareId);
}

double PricingService::getHardwarePrice(const std::string& hardwareId) const {
    auto it = config_.hardwarePrices.find(hardwareId);
    return (it != config_.hardwarePrices.end()) ? it->second : 0.0;
}

void PricingService::setCurrency(const std::string& currency, const std::string& symbol) {
    config_.currency = currency;
    if (!symbol.empty()) {
        config_.currencySymbol = symbol;
    }
    notifyPriceUpdated("Currency changed to: " + currency);
}

double PricingService::calculateVolumeDiscount(double totalValue) const {
    if (totalValue >= config_.volumeDiscountThreshold) {
        return totalValue * config_.volumeDiscountRate;
    }
    return 0.0;
}

double PricingService::calculateLoyaltyDiscount(double totalValue, bool isLoyalCustomer) const {
    if (isLoyalCustomer) {
        return totalValue * config_.loyaltyDiscountRate;
    }
    return 0.0;
}

PriceCalculation PricingService::applyDiscount(const PriceCalculation& original, double discountPercentage, 
                                              const std::string& reason) const {
    PriceCalculation result = original;
    double discountAmount = result.totalPrice * (discountPercentage / 100.0);
    result.discountAmount += discountAmount;
    result.totalPrice -= discountAmount;
    
    if (!reason.empty()) {
        result.notes.push_back("Discount applied: " + reason);
    }
    
    return result;
}

PriceCalculation PricingService::applyMarkup(const PriceCalculation& original, double markupPercentage,
                                            const std::string& reason) const {
    PriceCalculation result = original;
    double markupAmount = result.totalPrice * (markupPercentage / 100.0);
    result.overheadCost += markupAmount;
    result.totalPrice += markupAmount;
    
    if (!reason.empty()) {
        result.notes.push_back("Markup applied: " + reason);
    }
    
    return result;
}

void PricingService::recordPriceCalculation(const std::string& itemId, const PriceCalculation& calculation) {
    priceHistory_[itemId].push_back(calculation);
    
    // Limit history size
    const size_t maxHistorySize = 100;
    if (priceHistory_[itemId].size() > maxHistorySize) {
        priceHistory_[itemId].erase(priceHistory_[itemId].begin());
    }
}

std::vector<PriceCalculation> PricingService::getPriceHistory(const std::string& itemId) const {
    auto it = priceHistory_.find(itemId);
    return (it != priceHistory_.end()) ? it->second : std::vector<PriceCalculation>{};
}

std::optional<PriceCalculation> PricingService::getLatestPrice(const std::string& itemId) const {
    auto history = getPriceHistory(itemId);
    return history.empty() ? std::nullopt : std::make_optional(history.back());
}

void PricingService::clearPriceHistory(const std::string& itemId) {
    if (itemId.empty()) {
        priceHistory_.clear();
    } else {
        priceHistory_.erase(itemId);
    }
}

std::string PricingService::formatPrice(double price) const {
    std::stringstream ss;
    ss << config_.currencySymbol << std::fixed << std::setprecision(config_.decimalPlaces) << price;
    return ss.str();
}

double PricingService::parsePrice(const std::string& priceString) const {
    std::string cleanString = priceString;
    
    // Remove currency symbol
    size_t symbolPos = cleanString.find(config_.currencySymbol);
    if (symbolPos != std::string::npos) {
        cleanString.erase(symbolPos, config_.currencySymbol.length());
    }
    
    // Remove spaces and commas
    cleanString.erase(std::remove(cleanString.begin(), cleanString.end(), ' '), cleanString.end());
    cleanString.erase(std::remove(cleanString.begin(), cleanString.end(), ','), cleanString.end());
    
    try {
        return std::stod(cleanString);
    } catch (const std::exception& e) {
        notifyError("Failed to parse price: " + priceString);
        return 0.0;
    }
}

double PricingService::estimateComplexity(const CatalogItem& item) {
    double complexity = 1.0; // Base complexity
    
    // Size factor
    auto dims = item.getDimensions();
    if (dims.isValid()) {
        double volume = dims.volume() / 1000000000.0; // Convert mm³ to m³
        complexity += volume * 2.0; // Larger items are more complex
    }
    
    // Category factor
    std::string category = item.getCategory();
    if (category.find("corner") != std::string::npos) {
        complexity *= 1.5; // Corner items are more complex
    }
    if (category.find("tall") != std::string::npos) {
        complexity *= 1.3; // Tall items are more complex
    }
    
    return complexity;
}

double PricingService::calculateSurfaceArea(const CatalogItem& item) {
    auto dims = item.getDimensions();
    if (!dims.isValid()) {
        return 0.0;
    }
    
    // Calculate surface area of a box (simplified)
    double area = 2.0 * (dims.width * dims.height + dims.width * dims.depth + dims.height * dims.depth);
    return area / 1000000.0; // Convert mm² to m²
}

void PricingService::applyOverheadAndProfit(PriceCalculation& calculation) const {
    double subtotal = calculation.basePrice + calculation.materialCost + 
                     calculation.laborCost + calculation.hardwareCost;
    
    calculation.overheadCost += subtotal * config_.overheadPercentage;
    
    // Profit margin is typically included in the base price or applied as markup
    // For now, we'll include it in overhead
    calculation.overheadCost += subtotal * config_.profitMargin;
}

void PricingService::calculateTaxes(PriceCalculation& calculation) const {
    double taxableAmount = calculation.basePrice + calculation.materialCost + 
                          calculation.laborCost + calculation.hardwareCost + 
                          calculation.overheadCost - calculation.discountAmount;
    
    calculation.taxAmount = taxableAmount * config_.taxRate;
}

void PricingService::notifyPriceUpdated(const std::string& message) {
    if (priceUpdatedCallback_) {
        priceUpdatedCallback_(message);
    }
}

void PricingService::notifyError(const std::string& error) const {
    if (errorCallback_) {
        errorCallback_(error);
    }
}

void PricingService::initializeDefaultPricing() {
    // Default material prices (per m²)
    config_.materialPrices["melamine"] = 25.0;
    config_.materialPrices["plywood"] = 35.0;
    config_.materialPrices["mdf"] = 20.0;
    config_.materialPrices["solid_wood"] = 80.0;
    config_.materialPrices["laminate"] = 30.0;
    config_.materialPrices["veneer"] = 45.0;
    
    // Default hardware prices (per unit)
    config_.hardwarePrices["hinge"] = 5.0;
    config_.hardwarePrices["handle"] = 8.0;
    config_.hardwarePrices["knob"] = 6.0;
    config_.hardwarePrices["slide"] = 15.0;
    config_.hardwarePrices["soft_close"] = 12.0;
    
    // Default labor multipliers by category
    config_.laborMultipliers["base_cabinets"] = 1.0;
    config_.laborMultipliers["wall_cabinets"] = 0.8;
    config_.laborMultipliers["tall_cabinets"] = 1.5;
    config_.laborMultipliers["corner_cabinets"] = 1.8;
    config_.laborMultipliers["countertops"] = 0.6;
    config_.laborMultipliers["appliances"] = 0.3;
}

} // namespace Services
} // namespace KitchenCAD