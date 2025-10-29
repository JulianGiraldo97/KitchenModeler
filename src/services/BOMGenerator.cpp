#include "BOMGenerator.h"
#include "../utils/Logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <cmath>

namespace KitchenCAD {
namespace Services {

// BOMItem implementation
nlohmann::json BOMItem::toJson() const {
    nlohmann::json json;
    json["itemId"] = itemId;
    json["catalogItemId"] = catalogItemId;
    json["name"] = name;
    json["category"] = category;
    json["quantity"] = quantity;
    json["dimensions"] = {
        {"width", dimensions.width},
        {"height", dimensions.height},
        {"depth", dimensions.depth}
    };
    json["materialId"] = materialId;
    json["materialName"] = materialName;
    json["unitPrice"] = unitPrice;
    json["materialCost"] = materialCost;
    json["totalPrice"] = totalPrice;
    json["supplier"] = supplier;
    json["supplierCode"] = supplierCode;
    json["notes"] = notes;
    json["cutInfo"] = {
        {"length", cutInfo.length},
        {"width", cutInfo.width},
        {"thickness", cutInfo.thickness},
        {"material", cutInfo.material},
        {"sheetCount", cutInfo.sheetCount},
        {"wastePercentage", cutInfo.wastePercentage}
    };
    return json;
}

void BOMItem::fromJson(const nlohmann::json& json) {
    itemId = json.value("itemId", "");
    catalogItemId = json.value("catalogItemId", "");
    name = json.value("name", "");
    category = json.value("category", "");
    quantity = json.value("quantity", 0);
    
    if (json.contains("dimensions")) {
        auto dims = json["dimensions"];
        dimensions.width = dims.value("width", 0.0);
        dimensions.height = dims.value("height", 0.0);
        dimensions.depth = dims.value("depth", 0.0);
    }
    
    materialId = json.value("materialId", "");
    materialName = json.value("materialName", "");
    unitPrice = json.value("unitPrice", 0.0);
    materialCost = json.value("materialCost", 0.0);
    totalPrice = json.value("totalPrice", 0.0);
    supplier = json.value("supplier", "");
    supplierCode = json.value("supplierCode", "");
    notes = json.value("notes", "");
    
    if (json.contains("cutInfo")) {
        auto cut = json["cutInfo"];
        cutInfo.length = cut.value("length", 0.0);
        cutInfo.width = cut.value("width", 0.0);
        cutInfo.thickness = cut.value("thickness", 0.0);
        cutInfo.material = cut.value("material", "");
        cutInfo.sheetCount = cut.value("sheetCount", 0);
        cutInfo.wastePercentage = cut.value("wastePercentage", 0.0);
    }
}

// BillOfMaterials implementation
void BillOfMaterials::addItem(const BOMItem& item) {
    items_.push_back(item);
    recalculateTotals();
}

void BillOfMaterials::removeItem(const std::string& itemId) {
    items_.erase(std::remove_if(items_.begin(), items_.end(),
        [&itemId](const BOMItem& item) { return item.itemId == itemId; }), items_.end());
    recalculateTotals();
}

void BillOfMaterials::updateItem(const BOMItem& item) {
    auto it = std::find_if(items_.begin(), items_.end(),
        [&item](const BOMItem& existing) { return existing.itemId == item.itemId; });
    
    if (it != items_.end()) {
        *it = item;
        recalculateTotals();
    }
}

BOMItem* BillOfMaterials::getItem(const std::string& itemId) {
    auto it = std::find_if(items_.begin(), items_.end(),
        [&itemId](const BOMItem& item) { return item.itemId == itemId; });
    return (it != items_.end()) ? &(*it) : nullptr;
}

const BOMItem* BillOfMaterials::getItem(const std::string& itemId) const {
    auto it = std::find_if(items_.begin(), items_.end(),
        [&itemId](const BOMItem& item) { return item.itemId == itemId; });
    return (it != items_.end()) ? &(*it) : nullptr;
}

std::unordered_map<std::string, double> BillOfMaterials::getCostsByCategory() const {
    std::unordered_map<std::string, double> costs;
    
    for (const auto& item : items_) {
        costs[item.category] += item.totalPrice;
    }
    
    return costs;
}

std::unordered_map<std::string, int> BillOfMaterials::getQuantitiesByCategory() const {
    std::unordered_map<std::string, int> quantities;
    
    for (const auto& item : items_) {
        quantities[item.category] += item.quantity;
    }
    
    return quantities;
}

std::vector<BOMItem> BillOfMaterials::getItemsByCategory(const std::string& category) const {
    std::vector<BOMItem> result;
    
    for (const auto& item : items_) {
        if (item.category == category) {
            result.push_back(item);
        }
    }
    
    return result;
}

std::vector<BOMItem> BillOfMaterials::getMostExpensiveItems(size_t count) const {
    std::vector<BOMItem> sorted = items_;
    
    std::sort(sorted.begin(), sorted.end(),
        [](const BOMItem& a, const BOMItem& b) {
            return a.totalPrice > b.totalPrice;
        });
    
    if (sorted.size() > count) {
        sorted.resize(count);
    }
    
    return sorted;
}

bool BillOfMaterials::exportToCSV(const std::string& filename) const {
    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        
        // Header
        file << "Item ID,Name,Category,Quantity,Unit Price,Material Cost,Total Price,Supplier,Notes\n";
        
        // Items
        for (const auto& item : items_) {
            file << item.itemId << ","
                 << "\"" << item.name << "\","
                 << item.category << ","
                 << item.quantity << ","
                 << std::fixed << std::setprecision(2) << item.unitPrice << ","
                 << std::fixed << std::setprecision(2) << item.materialCost << ","
                 << std::fixed << std::setprecision(2) << item.totalPrice << ","
                 << "\"" << item.supplier << "\","
                 << "\"" << item.notes << "\"\n";
        }
        
        // Totals
        file << "\n";
        file << "Material Cost,," << std::fixed << std::setprecision(2) << materialCost_ << "\n";
        file << "Labor Cost,," << std::fixed << std::setprecision(2) << laborCost_ << "\n";
        file << "Overhead Cost,," << std::fixed << std::setprecision(2) << overheadCost_ << "\n";
        file << "Tax,," << std::fixed << std::setprecision(2) << getTaxAmount() << "\n";
        file << "Grand Total,," << std::fixed << std::setprecision(2) << getGrandTotal() << "\n";
        
        return true;
        
    } catch (const std::exception& e) {
        return false;
    }
}

bool BillOfMaterials::exportToJSON(const std::string& filename) const {
    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        
        file << toJson().dump(2);
        return true;
        
    } catch (const std::exception& e) {
        return false;
    }
}

std::string BillOfMaterials::generateReport() const {
    std::stringstream report;
    
    report << "BILL OF MATERIALS REPORT\n";
    report << "========================\n\n";
    
    report << "Project: " << projectName_ << "\n";
    report << "Customer: " << customerName_ << "\n";
    report << "Generated: " << std::chrono::system_clock::to_time_t(generatedAt_) << "\n\n";
    
    report << "ITEMS:\n";
    report << "------\n";
    
    for (const auto& item : items_) {
        report << item.name << " (" << item.category << ")\n";
        report << "  Quantity: " << item.quantity << "\n";
        report << "  Unit Price: " << std::fixed << std::setprecision(2) << item.unitPrice << " " << currency_ << "\n";
        report << "  Total: " << std::fixed << std::setprecision(2) << item.totalPrice << " " << currency_ << "\n\n";
    }
    
    report << "SUMMARY:\n";
    report << "--------\n";
    report << "Material Cost: " << std::fixed << std::setprecision(2) << materialCost_ << " " << currency_ << "\n";
    report << "Labor Cost: " << std::fixed << std::setprecision(2) << laborCost_ << " " << currency_ << "\n";
    report << "Overhead: " << std::fixed << std::setprecision(2) << overheadCost_ << " " << currency_ << "\n";
    report << "Tax: " << std::fixed << std::setprecision(2) << getTaxAmount() << " " << currency_ << "\n";
    report << "GRAND TOTAL: " << std::fixed << std::setprecision(2) << getGrandTotal() << " " << currency_ << "\n";
    
    return report.str();
}

nlohmann::json BillOfMaterials::toJson() const {
    nlohmann::json json;
    
    json["projectId"] = projectId_;
    json["projectName"] = projectName_;
    json["customerName"] = customerName_;
    json["currency"] = currency_;
    json["generatedAt"] = generatedAt_.time_since_epoch().count();
    json["notes"] = notes_;
    
    json["costs"] = {
        {"materialCost", materialCost_},
        {"laborCost", laborCost_},
        {"overheadCost", overheadCost_},
        {"taxRate", taxRate_},
        {"totalCost", totalCost_},
        {"grandTotal", getGrandTotal()}
    };
    
    nlohmann::json itemsArray = nlohmann::json::array();
    for (const auto& item : items_) {
        itemsArray.push_back(item.toJson());
    }
    json["items"] = itemsArray;
    
    return json;
}

void BillOfMaterials::fromJson(const nlohmann::json& json) {
    projectId_ = json.value("projectId", "");
    projectName_ = json.value("projectName", "");
    customerName_ = json.value("customerName", "");
    currency_ = json.value("currency", "USD");
    notes_ = json.value("notes", "");
    
    if (json.contains("generatedAt")) {
        auto timePoint = std::chrono::system_clock::time_point(
            std::chrono::system_clock::duration(json["generatedAt"].get<long long>()));
        generatedAt_ = timePoint;
    }
    
    if (json.contains("costs")) {
        auto costs = json["costs"];
        materialCost_ = costs.value("materialCost", 0.0);
        laborCost_ = costs.value("laborCost", 0.0);
        overheadCost_ = costs.value("overheadCost", 0.0);
        taxRate_ = costs.value("taxRate", 0.0);
        totalCost_ = costs.value("totalCost", 0.0);
    }
    
    items_.clear();
    if (json.contains("items")) {
        for (const auto& itemJson : json["items"]) {
            BOMItem item;
            item.fromJson(itemJson);
            items_.push_back(item);
        }
    }
}

void BillOfMaterials::recalculateTotals() {
    materialCost_ = 0.0;
    
    for (const auto& item : items_) {
        materialCost_ += item.totalPrice;
    }
    
    totalCost_ = materialCost_ + laborCost_ + overheadCost_;
}

// BOMGenerator implementation
BOMGenerator::BOMGenerator(const GenerationOptions& options)
    : defaultOptions_(options)
{
    initializeDefaultPricing();
}

std::unique_ptr<BillOfMaterials> BOMGenerator::generateBOM(const Project& project,
                                                          const std::function<std::shared_ptr<CatalogItem>(const std::string&)>& catalogLookup,
                                                          const GenerationOptions& options) {
    auto bom = std::make_unique<BillOfMaterials>();
    
    // Set project metadata
    bom->setProjectId(project.getId());
    bom->setProjectName(project.getName());
    bom->setCurrency(options.currency);
    
    notifyProgress("Starting BOM generation for project: " + project.getName());
    
    // Process all objects in the project
    const auto& objects = project.getObjects();
    size_t processedCount = 0;
    
    for (const auto& object : objects) {
        notifyProgress("Processing object " + std::to_string(++processedCount) + " of " + std::to_string(objects.size()));
        
        auto catalogItem = catalogLookup(object->getCatalogItemId());
        if (!catalogItem) {
            notifyError("Catalog item not found: " + object->getCatalogItemId());
            continue;
        }
        
        BOMItem bomItem = processSceneObject(*object, catalogItem, options);
        if (!bomItem.itemId.empty()) {
            bom->addItem(bomItem);
        }
    }
    
    // Group similar items if requested
    if (options.groupSimilarItems) {
        notifyProgress("Grouping similar items");
        groupSimilarItems(*bom);
    }
    
    // Calculate labor costs
    if (options.includeLabor) {
        notifyProgress("Calculating labor costs");
        double laborCost = calculateLaborCost(*bom, options);
        bom->setLaborCost(laborCost);
    }
    
    // Calculate overhead
    double overheadCost = bom->getMaterialCost() * options.overheadPercentage;
    bom->setOverheadCost(overheadCost);
    
    // Set tax rate
    bom->setTaxRate(options.taxRate);
    
    // Optimize cuts if requested
    if (options.optimizeCuts) {
        notifyProgress("Optimizing cuts");
        optimizeCuts(*bom, options.cutSettings);
    }
    
    notifyProgress("BOM generation completed");
    return bom;
}

std::unique_ptr<BillOfMaterials> BOMGenerator::generateBOMFromObjects(const std::vector<const SceneObject*>& objects,
                                                                     const std::function<std::shared_ptr<CatalogItem>(const std::string&)>& catalogLookup,
                                                                     const GenerationOptions& options) {
    auto bom = std::make_unique<BillOfMaterials>();
    bom->setCurrency(options.currency);
    
    notifyProgress("Starting BOM generation for " + std::to_string(objects.size()) + " objects");
    
    size_t processedCount = 0;
    for (const auto* object : objects) {
        notifyProgress("Processing object " + std::to_string(++processedCount) + " of " + std::to_string(objects.size()));
        
        auto catalogItem = catalogLookup(object->getCatalogItemId());
        if (!catalogItem) {
            notifyError("Catalog item not found: " + object->getCatalogItemId());
            continue;
        }
        
        BOMItem bomItem = processSceneObject(*object, catalogItem, options);
        if (!bomItem.itemId.empty()) {
            bom->addItem(bomItem);
        }
    }
    
    // Apply post-processing
    if (options.groupSimilarItems) {
        groupSimilarItems(*bom);
    }
    
    if (options.includeLabor) {
        double laborCost = calculateLaborCost(*bom, options);
        bom->setLaborCost(laborCost);
    }
    
    double overheadCost = bom->getMaterialCost() * options.overheadPercentage;
    bom->setOverheadCost(overheadCost);
    bom->setTaxRate(options.taxRate);
    
    if (options.optimizeCuts) {
        optimizeCuts(*bom, options.cutSettings);
    }
    
    notifyProgress("BOM generation completed");
    return bom;
}

CutOptimization BOMGenerator::optimizeCuts(BillOfMaterials& bom, const GenerationOptions::CutSettings& settings) {
    CutOptimization result;
    
    // Group items by material and thickness
    std::unordered_map<std::string, std::vector<BOMItem*>> materialGroups;
    
    for (auto& item : bom.items_) {
        if (item.cutInfo.thickness > 0) {
            std::string key = item.cutInfo.material + "_" + std::to_string(item.cutInfo.thickness);
            materialGroups[key].push_back(&item);
        }
    }
    
    // Optimize each material group
    for (auto& group : materialGroups) {
        auto sheets = optimizeCutLayout(group.second, settings);
        result.sheets.insert(result.sheets.end(), sheets.begin(), sheets.end());
    }
    
    // Calculate totals
    result.totalSheets = result.sheets.size();
    result.totalWaste = 0.0;
    result.totalCost = 0.0;
    
    for (const auto& sheet : result.sheets) {
        result.totalWaste += (1.0 - sheet.utilization) * sheet.width * sheet.height;
        // TODO: Calculate sheet cost based on material pricing
    }
    
    return result;
}

double BOMGenerator::calculateLaborCost(const BillOfMaterials& bom, const GenerationOptions& options) {
    double totalHours = 0.0;
    
    for (const auto& item : bom.getItems()) {
        // Look up labor time in configuration
        auto it = pricingConfig_.laborTimes.find(item.category);
        double hoursPerItem = (it != pricingConfig_.laborTimes.end()) ? it->second : 1.0; // Default 1 hour
        
        totalHours += hoursPerItem * item.quantity;
    }
    
    return totalHours * options.laborRatePerHour;
}

void BOMGenerator::updatePricing(BillOfMaterials& bom) {
    for (auto& item : bom.items_) {
        calculateItemPricing(item, defaultOptions_);
    }
    bom.recalculateTotals();
}

std::vector<std::string> BOMGenerator::validateBOM(const BillOfMaterials& bom) const {
    std::vector<std::string> issues;
    
    if (bom.getItems().empty()) {
        issues.push_back("BOM contains no items");
    }
    
    for (const auto& item : bom.getItems()) {
        if (item.name.empty()) {
            issues.push_back("Item " + item.itemId + " has no name");
        }
        
        if (item.quantity <= 0) {
            issues.push_back("Item " + item.itemId + " has invalid quantity");
        }
        
        if (item.unitPrice < 0) {
            issues.push_back("Item " + item.itemId + " has negative unit price");
        }
        
        if (!item.dimensions.isValid()) {
            issues.push_back("Item " + item.itemId + " has invalid dimensions");
        }
    }
    
    return issues;
}

double BOMGenerator::calculateMaterialArea(const CatalogItem& item, const MaterialProperties& material) {
    auto dims = item.getDimensions();
    if (!dims.isValid()) {
        return 0.0;
    }
    
    // Calculate surface area (simplified - assumes box shape)
    double area = 2.0 * (dims.width * dims.height + dims.width * dims.depth + dims.height * dims.depth);
    return area / 1000000.0; // Convert mm² to m²
}

int BOMGenerator::calculateHardwareCount(const CatalogItem& item) {
    // Simplified hardware calculation based on category
    std::string category = item.getCategory();
    
    if (category.find("cabinet") != std::string::npos) {
        return 4; // Hinges, handles, etc.
    } else if (category.find("drawer") != std::string::npos) {
        return 2; // Slides
    }
    
    return 1; // Default
}

double BOMGenerator::estimateLaborTime(const CatalogItem& item) {
    // Simplified labor estimation based on category and size
    std::string category = item.getCategory();
    auto dims = item.getDimensions();
    
    double baseTime = 1.0; // hours
    
    if (category.find("cabinet") != std::string::npos) {
        baseTime = 2.0;
    } else if (category.find("countertop") != std::string::npos) {
        baseTime = 1.5;
    }
    
    // Adjust for size
    if (dims.isValid()) {
        double volume = dims.volume() / 1000000000.0; // Convert mm³ to m³
        baseTime *= (1.0 + volume * 0.5); // Larger items take more time
    }
    
    return baseTime;
}

void BOMGenerator::groupSimilarItems(BillOfMaterials& bom) const {
    std::vector<BOMItem> groupedItems;
    std::unordered_map<std::string, size_t> itemGroups;
    
    for (const auto& item : bom.getItems()) {
        // Create grouping key based on catalog item and material
        std::string key = item.catalogItemId + "_" + item.materialId;
        
        auto it = itemGroups.find(key);
        if (it != itemGroups.end()) {
            // Add to existing group
            groupedItems[it->second].quantity += item.quantity;
            groupedItems[it->second].totalPrice += item.totalPrice;
        } else {
            // Create new group
            itemGroups[key] = groupedItems.size();
            groupedItems.push_back(item);
        }
    }
    
    // Replace items in BOM
    bom.clear();
    for (const auto& item : groupedItems) {
        bom.addItem(item);
    }
}

std::string BOMGenerator::generateItemDescription(const CatalogItem& item, const MaterialProperties& material) {
    std::stringstream desc;
    desc << item.getName();
    
    if (!material.name.empty()) {
        desc << " - " << material.name;
    }
    
    auto dims = item.getDimensions();
    if (dims.isValid()) {
        desc << " (" << dims.width << "x" << dims.height << "x" << dims.depth << "mm)";
    }
    
    return desc.str();
}

BOMItem BOMGenerator::processSceneObject(const SceneObject& object,
                                        const std::shared_ptr<CatalogItem>& catalogItem,
                                        const GenerationOptions& options) {
    BOMItem item;
    
    item.itemId = object.getId();
    item.catalogItemId = catalogItem->getId();
    item.name = catalogItem->getName();
    item.category = catalogItem->getCategory();
    item.quantity = 1; // Each scene object represents one item
    item.dimensions = catalogItem->getDimensions();
    
    // Material information
    const auto& material = object.getMaterial();
    item.materialId = material.id;
    item.materialName = material.name;
    
    // Calculate pricing
    calculateItemPricing(item, options);
    
    // Cut information for optimization
    if (options.optimizeCuts) {
        item.cutInfo.length = item.dimensions.width;
        item.cutInfo.width = item.dimensions.height;
        item.cutInfo.thickness = item.dimensions.depth;
        item.cutInfo.material = material.name;
    }
    
    return item;
}

void BOMGenerator::calculateItemPricing(BOMItem& item, const GenerationOptions& options) {
    // Base price from catalog
    item.unitPrice = 100.0; // TODO: Get from catalog item
    
    // Material cost
    auto materialIt = pricingConfig_.materialPrices.find(item.materialId);
    if (materialIt != pricingConfig_.materialPrices.end()) {
        double area = item.dimensions.width * item.dimensions.height / 1000000.0; // Convert to m²
        item.materialCost = area * materialIt->second;
    } else {
        item.materialCost = 0.0;
    }
    
    // Total price
    item.totalPrice = (item.unitPrice + item.materialCost) * item.quantity;
}

std::vector<CutOptimization::Sheet> BOMGenerator::optimizeCutLayout(const std::vector<BOMItem*>& items,
                                                                   const GenerationOptions::CutSettings& settings) {
    std::vector<CutOptimization::Sheet> sheets;
    
    // Simple bin packing algorithm (First Fit Decreasing)
    std::vector<BOMItem*> sortedItems = items;
    std::sort(sortedItems.begin(), sortedItems.end(),
        [](const BOMItem* a, const BOMItem* b) {
            return (a->cutInfo.length * a->cutInfo.width) > (b->cutInfo.length * b->cutInfo.width);
        });
    
    for (auto* item : sortedItems) {
        bool placed = false;
        
        // Try to place in existing sheet
        for (auto& sheet : sheets) {
            if (sheet.material == item->cutInfo.material) {
                // Simple placement check (could be improved with 2D bin packing)
                double usedArea = 0.0;
                for (auto* sheetItem : sheet.items) {
                    usedArea += sheetItem->cutInfo.length * sheetItem->cutInfo.width;
                }
                
                double itemArea = item->cutInfo.length * item->cutInfo.width;
                double sheetArea = sheet.width * sheet.height;
                
                if (usedArea + itemArea <= sheetArea * 0.9) { // 90% utilization limit
                    sheet.items.push_back(item);
                    sheet.utilization = (usedArea + itemArea) / sheetArea;
                    placed = true;
                    break;
                }
            }
        }
        
        // Create new sheet if not placed
        if (!placed) {
            CutOptimization::Sheet newSheet;
            newSheet.width = settings.sheetWidth;
            newSheet.height = settings.sheetHeight;
            newSheet.thickness = item->cutInfo.thickness;
            newSheet.material = item->cutInfo.material;
            newSheet.items.push_back(item);
            
            double itemArea = item->cutInfo.length * item->cutInfo.width;
            double sheetArea = newSheet.width * newSheet.height;
            newSheet.utilization = itemArea / sheetArea;
            
            sheets.push_back(newSheet);
        }
    }
    
    return sheets;
}

void BOMGenerator::notifyProgress(const std::string& message) {
    if (progressCallback_) {
        progressCallback_(message);
    }
}

void BOMGenerator::notifyError(const std::string& error) {
    if (errorCallback_) {
        errorCallback_(error);
    }
}

void BOMGenerator::initializeDefaultPricing() {
    // Default material prices (per m²)
    pricingConfig_.materialPrices["melamine"] = 25.0;
    pricingConfig_.materialPrices["plywood"] = 35.0;
    pricingConfig_.materialPrices["mdf"] = 20.0;
    pricingConfig_.materialPrices["solid_wood"] = 80.0;
    
    // Default hardware prices (per unit)
    pricingConfig_.hardwarePrices["hinge"] = 5.0;
    pricingConfig_.hardwarePrices["handle"] = 8.0;
    pricingConfig_.hardwarePrices["slide"] = 15.0;
    
    // Default labor times (hours per item)
    pricingConfig_.laborTimes["base_cabinets"] = 2.0;
    pricingConfig_.laborTimes["wall_cabinets"] = 1.5;
    pricingConfig_.laborTimes["tall_cabinets"] = 3.0;
    pricingConfig_.laborTimes["countertops"] = 1.0;
}

} // namespace Services
} // namespace KitchenCAD