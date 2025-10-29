#include "ConfigurationService.h"
#include "../utils/Logger.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <ctime>

namespace KitchenCAD {
namespace Services {

// ConfigValue implementation
std::string ConfigValue::toString() const {
    if (type_ == typeid(std::string).name()) {
        return getValue<std::string>();
    } else if (type_ == typeid(int).name()) {
        return std::to_string(getValue<int>());
    } else if (type_ == typeid(double).name()) {
        return std::to_string(getValue<double>());
    } else if (type_ == typeid(bool).name()) {
        return getValue<bool>() ? "true" : "false";
    }
    return "";
}

void ConfigValue::fromString(const std::string& str) {
    if (isReadOnly_) {
        throw std::runtime_error("Cannot modify read-only configuration value");
    }
    
    if (type_ == typeid(std::string).name()) {
        setValue<std::string>(str);
    } else if (type_ == typeid(int).name()) {
        setValue<int>(std::stoi(str));
    } else if (type_ == typeid(double).name()) {
        setValue<double>(std::stod(str));
    } else if (type_ == typeid(bool).name()) {
        setValue<bool>(str == "true" || str == "1");
    }
}

nlohmann::json ConfigValue::toJson() const {
    nlohmann::json json;
    json["type"] = type_;
    json["description"] = description_;
    json["readOnly"] = isReadOnly_;
    
    if (type_ == typeid(std::string).name()) {
        json["value"] = getValue<std::string>();
    } else if (type_ == typeid(int).name()) {
        json["value"] = getValue<int>();
    } else if (type_ == typeid(double).name()) {
        json["value"] = getValue<double>();
    } else if (type_ == typeid(bool).name()) {
        json["value"] = getValue<bool>();
    }
    
    return json;
}

void ConfigValue::fromJson(const nlohmann::json& json) {
    type_ = json.value("type", "");
    description_ = json.value("description", "");
    isReadOnly_ = json.value("readOnly", false);
    
    if (json.contains("value")) {
        if (type_ == typeid(std::string).name()) {
            setValue<std::string>(json["value"].get<std::string>());
        } else if (type_ == typeid(int).name()) {
            setValue<int>(json["value"].get<int>());
        } else if (type_ == typeid(double).name()) {
            setValue<double>(json["value"].get<double>());
        } else if (type_ == typeid(bool).name()) {
            setValue<bool>(json["value"].get<bool>());
        }
    }
}

// ConfigSection implementation
std::vector<std::string> ConfigSection::getKeys() const {
    std::vector<std::string> keys;
    keys.reserve(values_.size());
    
    for (const auto& pair : values_) {
        keys.push_back(pair.first);
    }
    
    return keys;
}

nlohmann::json ConfigSection::toJson() const {
    nlohmann::json json;
    json["name"] = name_;
    json["description"] = description_;
    
    nlohmann::json valuesJson;
    for (const auto& pair : values_) {
        valuesJson[pair.first] = pair.second.toJson();
    }
    json["values"] = valuesJson;
    
    return json;
}

void ConfigSection::fromJson(const nlohmann::json& json) {
    name_ = json.value("name", "");
    description_ = json.value("description", "");
    
    values_.clear();
    if (json.contains("values")) {
        for (const auto& item : json["values"].items()) {
            ConfigValue value;
            value.fromJson(item.value());
            values_[item.key()] = value;
        }
    }
}

// ConfigurationService implementation
ConfigurationService::ConfigurationService(const std::string& configFilePath)
    : configFilePath_(configFilePath)
{
    initializeDefaults();
    initializeLanguages();
    initializeCurrencies();
    initializeThemes();
    initializeUnitConversions();
    
    // Try to load existing configuration
    loadConfiguration();
    updateCachedConfigurations();
}

ConfigurationService::~ConfigurationService() {
    saveConfiguration();
}

bool ConfigurationService::loadConfiguration() {
    try {
        if (!std::filesystem::exists(configFilePath_)) {
            // Create default configuration file
            return saveConfiguration();
        }
        
        std::ifstream file(configFilePath_);
        if (!file.is_open()) {
            notifyError("Cannot open configuration file: " + configFilePath_);
            return false;
        }
        
        nlohmann::json configJson;
        file >> configJson;
        
        fromJson(configJson);
        updateCachedConfigurations();
        
        return true;
        
    } catch (const std::exception& e) {
        notifyError("Failed to load configuration: " + std::string(e.what()));
        return false;
    }
}

bool ConfigurationService::saveConfiguration() const {
    try {
        // Create directory if it doesn't exist
        std::filesystem::path configPath(configFilePath_);
        std::filesystem::create_directories(configPath.parent_path());
        
        std::ofstream file(configFilePath_);
        if (!file.is_open()) {
            notifyError("Cannot create configuration file: " + configFilePath_);
            return false;
        }
        
        nlohmann::json configJson = toJson();
        file << configJson.dump(2);
        
        return true;
        
    } catch (const std::exception& e) {
        notifyError("Failed to save configuration: " + std::string(e.what()));
        return false;
    }
}

void ConfigurationService::resetToDefaults() {
    sections_.clear();
    initializeDefaults();
    updateCachedConfigurations();
    saveConfiguration();
}

bool ConfigurationService::importConfiguration(const std::string& filePath) {
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            notifyError("Cannot open import file: " + filePath);
            return false;
        }
        
        nlohmann::json configJson;
        file >> configJson;
        
        fromJson(configJson);
        updateCachedConfigurations();
        saveConfiguration();
        
        return true;
        
    } catch (const std::exception& e) {
        notifyError("Failed to import configuration: " + std::string(e.what()));
        return false;
    }
}

bool ConfigurationService::exportConfiguration(const std::string& filePath) const {
    try {
        std::ofstream file(filePath);
        if (!file.is_open()) {
            notifyError("Cannot create export file: " + filePath);
            return false;
        }
        
        nlohmann::json configJson = toJson();
        file << configJson.dump(2);
        
        return true;
        
    } catch (const std::exception& e) {
        notifyError("Failed to export configuration: " + std::string(e.what()));
        return false;
    }
}

ConfigSection& ConfigurationService::getSection(const std::string& sectionName, const std::string& description) {
    auto it = sections_.find(sectionName);
    if (it == sections_.end()) {
        sections_[sectionName] = ConfigSection(sectionName, description);
    }
    return sections_[sectionName];
}

bool ConfigurationService::hasSection(const std::string& sectionName) const {
    return sections_.find(sectionName) != sections_.end();
}

void ConfigurationService::removeSection(const std::string& sectionName) {
    sections_.erase(sectionName);
}

std::vector<std::string> ConfigurationService::getSectionNames() const {
    std::vector<std::string> names;
    names.reserve(sections_.size());
    
    for (const auto& pair : sections_) {
        names.push_back(pair.first);
    }
    
    return names;
}

bool ConfigurationService::hasValue(const std::string& section, const std::string& key) const {
    auto it = sections_.find(section);
    if (it != sections_.end()) {
        return it->second.hasValue(key);
    }
    return false;
}

void ConfigurationService::removeValue(const std::string& section, const std::string& key) {
    auto it = sections_.find(section);
    if (it != sections_.end()) {
        it->second.removeValue(key);
        notifyValueChanged(section, key);
    }
}

void ConfigurationService::setLanguage(const std::string& languageCode) {
    const LanguageConfig* language = findLanguage(languageCode);
    if (language) {
        languageConfig_ = *language;
        setValue("localization", "language", languageCode, "Current language code");
        notifyLanguageChanged(languageCode);
    } else {
        notifyError("Language not found: " + languageCode);
    }
}

void ConfigurationService::addAvailableLanguage(const LanguageConfig& language) {
    // Remove existing language with same code
    availableLanguages_.erase(
        std::remove_if(availableLanguages_.begin(), availableLanguages_.end(),
            [&language](const LanguageConfig& existing) { return existing.code == language.code; }),
        availableLanguages_.end());
    
    availableLanguages_.push_back(language);
}

std::string ConfigurationService::getLocalizedString(const std::string& key, const std::string& defaultValue) const {
    // TODO: Implement proper localization system
    // For now, return the key or default value
    return defaultValue.empty() ? key : defaultValue;
}

void ConfigurationService::setUnitSystem(UnitConfig::System system) {
    unitConfig_.system = system;
    
    // Update unit preferences based on system
    switch (system) {
        case UnitConfig::System::Metric:
            unitConfig_.lengthUnit = "mm";
            unitConfig_.areaUnit = "m²";
            unitConfig_.volumeUnit = "m³";
            unitConfig_.weightUnit = "kg";
            unitConfig_.temperatureUnit = "°C";
            break;
        case UnitConfig::System::Imperial:
            unitConfig_.lengthUnit = "in";
            unitConfig_.areaUnit = "ft²";
            unitConfig_.volumeUnit = "ft³";
            unitConfig_.weightUnit = "lb";
            unitConfig_.temperatureUnit = "°F";
            break;
        case UnitConfig::System::Mixed:
            // Keep current settings
            break;
    }
    
    setValue("units", "system", static_cast<int>(system), "Unit system (0=Metric, 1=Imperial, 2=Mixed)");
    setValue("units", "lengthUnit", unitConfig_.lengthUnit, "Primary length unit");
    setValue("units", "areaUnit", unitConfig_.areaUnit, "Primary area unit");
    setValue("units", "volumeUnit", unitConfig_.volumeUnit, "Primary volume unit");
}

double ConfigurationService::convertUnit(double value, const std::string& fromUnit, const std::string& toUnit) const {
    if (fromUnit == toUnit) {
        return value;
    }
    
    // Convert to base unit (mm) first
    auto fromIt = unitConfig_.conversionFactors.find(fromUnit);
    if (fromIt == unitConfig_.conversionFactors.end()) {
        notifyError("Unknown unit: " + fromUnit);
        return value;
    }
    
    double baseValue = value * fromIt->second;
    
    // Convert from base unit to target unit
    auto toIt = unitConfig_.conversionFactors.find(toUnit);
    if (toIt == unitConfig_.conversionFactors.end()) {
        notifyError("Unknown unit: " + toUnit);
        return value;
    }
    
    return baseValue / toIt->second;
}

std::string ConfigurationService::formatWithUnit(double value, const std::string& unit) const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << value << " " << unit;
    return ss.str();
}

void ConfigurationService::setCurrency(const std::string& currencyCode) {
    const CurrencyConfig* currency = findCurrency(currencyCode);
    if (currency) {
        currencyConfig_ = *currency;
        setValue("currency", "code", currencyCode, "Current currency code");
        setValue("currency", "symbol", currency->symbol, "Currency symbol");
    } else {
        notifyError("Currency not found: " + currencyCode);
    }
}

std::string ConfigurationService::formatCurrency(double value) const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(currencyConfig_.decimalPlaces);
    
    if (currencyConfig_.symbolBefore) {
        ss << currencyConfig_.symbol << value;
    } else {
        ss << value << currencyConfig_.symbol;
    }
    
    return ss.str();
}

void ConfigurationService::setTheme(const std::string& themeName) {
    const ThemeConfig* theme = findTheme(themeName);
    if (theme) {
        themeConfig_ = *theme;
        setValue("theme", "name", themeName, "Current theme name");
        notifyThemeChanged(themeName);
    } else {
        notifyError("Theme not found: " + themeName);
    }
}

void ConfigurationService::addCustomTheme(const ThemeConfig& theme) {
    // Remove existing theme with same name
    availableThemes_.erase(
        std::remove_if(availableThemes_.begin(), availableThemes_.end(),
            [&theme](const ThemeConfig& existing) { return existing.name == theme.name; }),
        availableThemes_.end());
    
    availableThemes_.push_back(theme);
}

bool ConfigurationService::removeCustomTheme(const std::string& themeName) {
    auto it = std::find_if(availableThemes_.begin(), availableThemes_.end(),
        [&themeName](const ThemeConfig& theme) { return theme.name == themeName; });
    
    if (it != availableThemes_.end()) {
        availableThemes_.erase(it);
        return true;
    }
    return false;
}

std::vector<std::string> ConfigurationService::validateConfiguration() const {
    std::vector<std::string> issues;
    
    // Validate language
    if (!findLanguage(languageConfig_.code)) {
        issues.push_back("Invalid language code: " + languageConfig_.code);
    }
    
    // Validate currency
    if (!findCurrency(currencyConfig_.code)) {
        issues.push_back("Invalid currency code: " + currencyConfig_.code);
    }
    
    // Validate theme
    if (!findTheme(themeConfig_.name)) {
        issues.push_back("Invalid theme name: " + themeConfig_.name);
    }
    
    // Validate numeric ranges
    if (currencyConfig_.decimalPlaces < 0 || currencyConfig_.decimalPlaces > 10) {
        issues.push_back("Invalid decimal places for currency: " + std::to_string(currencyConfig_.decimalPlaces));
    }
    
    if (themeConfig_.fontSize < 6 || themeConfig_.fontSize > 72) {
        issues.push_back("Invalid font size: " + std::to_string(themeConfig_.fontSize));
    }
    
    return issues;
}

nlohmann::json ConfigurationService::toJson() const {
    nlohmann::json json;
    json["version"] = "1.0";
    json["lastSaved"] = std::chrono::system_clock::now().time_since_epoch().count();
    
    nlohmann::json sectionsJson;
    for (const auto& pair : sections_) {
        sectionsJson[pair.first] = pair.second.toJson();
    }
    json["sections"] = sectionsJson;
    
    return json;
}

void ConfigurationService::fromJson(const nlohmann::json& json) {
    sections_.clear();
    
    if (json.contains("sections")) {
        for (const auto& item : json["sections"].items()) {
            ConfigSection section;
            section.fromJson(item.value());
            sections_[item.key()] = section;
        }
    }
}

bool ConfigurationService::createBackup(const std::string& backupPath) const {
    std::string actualBackupPath = backupPath;
    if (actualBackupPath.empty()) {
        // Generate backup filename with timestamp
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        
        std::stringstream ss;
        ss << configFilePath_ << ".backup." 
           << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
        actualBackupPath = ss.str();
    }
    
    return exportConfiguration(actualBackupPath);
}

bool ConfigurationService::restoreFromBackup(const std::string& backupPath) {
    return importConfiguration(backupPath);
}

void ConfigurationService::initializeDefaults() {
    // Application settings
    auto& appSection = getSection("application", "Application Settings");
    appSection.setValue("autoSave", true, "Enable automatic saving");
    appSection.setValue("autoSaveInterval", 30, "Auto-save interval in seconds");
    appSection.setValue("maxRecentProjects", 10, "Maximum number of recent projects to remember");
    appSection.setValue("checkForUpdates", true, "Check for application updates");
    appSection.setValue("sendUsageStatistics", false, "Send anonymous usage statistics");
    
    // UI settings
    auto& uiSection = getSection("ui", "User Interface Settings");
    uiSection.setValue("showToolTips", true, "Show tool tips");
    uiSection.setValue("showStatusBar", true, "Show status bar");
    uiSection.setValue("showGrid", true, "Show grid in design canvas");
    uiSection.setValue("gridSize", 10.0, "Grid size in current units");
    uiSection.setValue("snapToGrid", true, "Snap objects to grid");
    uiSection.setValue("showDimensions", true, "Show dimensions while drawing");
    
    // Rendering settings
    auto& renderSection = getSection("rendering", "Rendering Settings");
    renderSection.setValue("antiAliasing", true, "Enable anti-aliasing");
    renderSection.setValue("shadows", true, "Enable shadows");
    renderSection.setValue("reflections", false, "Enable reflections");
    renderSection.setValue("maxFPS", 60, "Maximum frames per second");
    renderSection.setValue("vsync", true, "Enable vertical sync");
    
    // Project settings
    auto& projectSection = getSection("project", "Project Settings");
    projectSection.setValue("defaultRoomWidth", 4000.0, "Default room width in mm");
    projectSection.setValue("defaultRoomHeight", 2500.0, "Default room height in mm");
    projectSection.setValue("defaultRoomDepth", 3000.0, "Default room depth in mm");
    projectSection.setValue("createBackups", true, "Create project backups");
    projectSection.setValue("maxBackups", 5, "Maximum number of backups to keep");
    
    // Export settings
    auto& exportSection = getSection("export", "Export Settings");
    exportSection.setValue("defaultImageFormat", std::string("PNG"), "Default image export format");
    exportSection.setValue("imageQuality", 95, "Image export quality (0-100)");
    exportSection.setValue("includeMetadata", true, "Include metadata in exports");
    exportSection.setValue("defaultCADFormat", std::string("STEP"), "Default CAD export format");
}

void ConfigurationService::initializeLanguages() {
    // English
    LanguageConfig english;
    english.code = "en";
    english.name = "English";
    english.nativeName = "English";
    english.region = "US";
    availableLanguages_.push_back(english);
    
    // Spanish
    LanguageConfig spanish;
    spanish.code = "es";
    spanish.name = "Spanish";
    spanish.nativeName = "Español";
    spanish.region = "ES";
    spanish.dateFormat = "dd/MM/yyyy";
    spanish.numberFormat = "es-ES";
    availableLanguages_.push_back(spanish);
    
    // French
    LanguageConfig french;
    french.code = "fr";
    french.name = "French";
    french.nativeName = "Français";
    french.region = "FR";
    french.dateFormat = "dd/MM/yyyy";
    french.numberFormat = "fr-FR";
    availableLanguages_.push_back(french);
    
    // German
    LanguageConfig german;
    german.code = "de";
    german.name = "German";
    german.nativeName = "Deutsch";
    german.region = "DE";
    german.dateFormat = "dd.MM.yyyy";
    german.numberFormat = "de-DE";
    availableLanguages_.push_back(german);
    
    // Set default language
    languageConfig_ = english;
}

void ConfigurationService::initializeCurrencies() {
    // USD
    CurrencyConfig usd;
    usd.code = "USD";
    usd.symbol = "$";
    usd.name = "US Dollar";
    availableCurrencies_.push_back(usd);
    
    // EUR
    CurrencyConfig eur;
    eur.code = "EUR";
    eur.symbol = "€";
    eur.name = "Euro";
    eur.symbolBefore = false;
    availableCurrencies_.push_back(eur);
    
    // GBP
    CurrencyConfig gbp;
    gbp.code = "GBP";
    gbp.symbol = "£";
    gbp.name = "British Pound";
    availableCurrencies_.push_back(gbp);
    
    // JPY
    CurrencyConfig jpy;
    jpy.code = "JPY";
    jpy.symbol = "¥";
    jpy.name = "Japanese Yen";
    jpy.decimalPlaces = 0;
    availableCurrencies_.push_back(jpy);
    
    // Set default currency
    currencyConfig_ = usd;
}

void ConfigurationService::initializeThemes() {
    // Default light theme
    ThemeConfig light;
    light.name = "default";
    light.displayName = "Default Light";
    light.isDark = false;
    availableThemes_.push_back(light);
    
    // Dark theme
    ThemeConfig dark;
    dark.name = "dark";
    dark.displayName = "Dark";
    dark.isDark = true;
    dark.backgroundColor = "#2B2B2B";
    dark.textColor = "#FFFFFF";
    dark.primaryColor = "#3F51B5";
    availableThemes_.push_back(dark);
    
    // Blue theme
    ThemeConfig blue;
    blue.name = "blue";
    blue.displayName = "Blue";
    blue.primaryColor = "#1976D2";
    blue.secondaryColor = "#FFC107";
    blue.accentColor = "#FF5722";
    availableThemes_.push_back(blue);
    
    // Set default theme
    themeConfig_ = light;
}

void ConfigurationService::initializeUnitConversions() {
    // Length conversions (to mm)
    unitConfig_.conversionFactors["mm"] = 1.0;
    unitConfig_.conversionFactors["cm"] = 10.0;
    unitConfig_.conversionFactors["m"] = 1000.0;
    unitConfig_.conversionFactors["in"] = 25.4;
    unitConfig_.conversionFactors["ft"] = 304.8;
    
    // Area conversions (to mm²)
    unitConfig_.conversionFactors["mm²"] = 1.0;
    unitConfig_.conversionFactors["cm²"] = 100.0;
    unitConfig_.conversionFactors["m²"] = 1000000.0;
    unitConfig_.conversionFactors["in²"] = 645.16;
    unitConfig_.conversionFactors["ft²"] = 92903.04;
    
    // Volume conversions (to mm³)
    unitConfig_.conversionFactors["mm³"] = 1.0;
    unitConfig_.conversionFactors["cm³"] = 1000.0;
    unitConfig_.conversionFactors["m³"] = 1000000000.0;
    unitConfig_.conversionFactors["in³"] = 16387.064;
    unitConfig_.conversionFactors["ft³"] = 28316846.592;
}

void ConfigurationService::updateCachedConfigurations() {
    // Update language
    std::string languageCode = getValue<std::string>("localization", "language", "en");
    const LanguageConfig* language = findLanguage(languageCode);
    if (language) {
        languageConfig_ = *language;
    }
    
    // Update currency
    std::string currencyCode = getValue<std::string>("currency", "code", "USD");
    const CurrencyConfig* currency = findCurrency(currencyCode);
    if (currency) {
        currencyConfig_ = *currency;
    }
    
    // Update theme
    std::string themeName = getValue<std::string>("theme", "name", "default");
    const ThemeConfig* theme = findTheme(themeName);
    if (theme) {
        themeConfig_ = *theme;
    }
    
    // Update unit system
    int systemInt = getValue<int>("units", "system", 0);
    unitConfig_.system = static_cast<UnitConfig::System>(systemInt);
    unitConfig_.lengthUnit = getValue<std::string>("units", "lengthUnit", "mm");
    unitConfig_.areaUnit = getValue<std::string>("units", "areaUnit", "m²");
    unitConfig_.volumeUnit = getValue<std::string>("units", "volumeUnit", "m³");
}

void ConfigurationService::notifyValueChanged(const std::string& section, const std::string& key) {
    if (valueChangedCallback_) {
        valueChangedCallback_(section, key);
    }
}

void ConfigurationService::notifyLanguageChanged(const std::string& languageCode) {
    if (languageChangedCallback_) {
        languageChangedCallback_(languageCode);
    }
}

void ConfigurationService::notifyThemeChanged(const std::string& themeName) {
    if (themeChangedCallback_) {
        themeChangedCallback_(themeName);
    }
}

void ConfigurationService::notifyError(const std::string& error) const {
    if (errorCallback_) {
        errorCallback_(error);
    }
}

const ConfigurationService::LanguageConfig* ConfigurationService::findLanguage(const std::string& code) const {
    auto it = std::find_if(availableLanguages_.begin(), availableLanguages_.end(),
        [&code](const LanguageConfig& lang) { return lang.code == code; });
    return (it != availableLanguages_.end()) ? &(*it) : nullptr;
}

const ConfigurationService::CurrencyConfig* ConfigurationService::findCurrency(const std::string& code) const {
    auto it = std::find_if(availableCurrencies_.begin(), availableCurrencies_.end(),
        [&code](const CurrencyConfig& curr) { return curr.code == code; });
    return (it != availableCurrencies_.end()) ? &(*it) : nullptr;
}

const ConfigurationService::ThemeConfig* ConfigurationService::findTheme(const std::string& name) const {
    auto it = std::find_if(availableThemes_.begin(), availableThemes_.end(),
        [&name](const ThemeConfig& theme) { return theme.name == name; });
    return (it != availableThemes_.end()) ? &(*it) : nullptr;
}

} // namespace Services
} // namespace KitchenCAD