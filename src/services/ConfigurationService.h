#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <any>
#include <typeinfo>
#include <memory>
#include <nlohmann/json.hpp>

namespace KitchenCAD {
namespace Services {

/**
 * @brief Configuration value with type information
 */
class ConfigValue {
private:
    std::any value_;
    std::string type_;
    std::string description_;
    bool isReadOnly_;

public:
    ConfigValue() : isReadOnly_(false) {}
    
    template<typename T>
    ConfigValue(const T& value, const std::string& description = "", bool readOnly = false)
        : value_(value), type_(typeid(T).name()), description_(description), isReadOnly_(readOnly) {}
    
    template<typename T>
    T getValue() const {
        try {
            return std::any_cast<T>(value_);
        } catch (const std::bad_any_cast& e) {
            throw std::runtime_error("Invalid type cast for config value");
        }
    }
    
    template<typename T>
    void setValue(const T& value) {
        if (isReadOnly_) {
            throw std::runtime_error("Cannot modify read-only configuration value");
        }
        value_ = value;
        type_ = typeid(T).name();
    }
    
    const std::string& getType() const { return type_; }
    const std::string& getDescription() const { return description_; }
    bool isReadOnly() const { return isReadOnly_; }
    void setReadOnly(bool readOnly) { isReadOnly_ = readOnly; }
    
    std::string toString() const;
    void fromString(const std::string& str);
    
    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& json);
};

/**
 * @brief Configuration section grouping related settings
 */
class ConfigSection {
private:
    std::string name_;
    std::string description_;
    std::unordered_map<std::string, ConfigValue> values_;
    
public:
    ConfigSection() = default;
    ConfigSection(const std::string& name, const std::string& description = "")
        : name_(name), description_(description) {}
    
    const std::string& getName() const { return name_; }
    const std::string& getDescription() const { return description_; }
    
    template<typename T>
    void setValue(const std::string& key, const T& value, const std::string& description = "", bool readOnly = false) {
        values_[key] = ConfigValue(value, description, readOnly);
    }
    
    template<typename T>
    T getValue(const std::string& key, const T& defaultValue = T{}) const {
        auto it = values_.find(key);
        if (it != values_.end()) {
            try {
                return it->second.getValue<T>();
            } catch (const std::exception& e) {
                return defaultValue;
            }
        }
        return defaultValue;
    }
    
    bool hasValue(const std::string& key) const {
        return values_.find(key) != values_.end();
    }
    
    void removeValue(const std::string& key) {
        values_.erase(key);
    }
    
    std::vector<std::string> getKeys() const;
    const std::unordered_map<std::string, ConfigValue>& getValues() const { return values_; }
    
    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& json);
};

/**
 * @brief Configuration service for user preferences and application settings
 * 
 * This service manages all configuration settings including user preferences,
 * application settings, localization, and system configuration.
 * Implements requirements 15.1, 15.2, 15.3, 15.4, 15.5
 */
class ConfigurationService {
public:
    /**
     * @brief Language configuration
     */
    struct LanguageConfig {
        std::string code = "en";
        std::string name = "English";
        std::string nativeName = "English";
        std::string region = "US";
        bool rightToLeft = false;
        std::string dateFormat = "MM/dd/yyyy";
        std::string timeFormat = "HH:mm:ss";
        std::string numberFormat = "en-US";
    };
    
    /**
     * @brief Unit system configuration
     */
    struct UnitConfig {
        enum class System {
            Metric,
            Imperial,
            Mixed
        } system = System::Metric;
        
        std::string lengthUnit = "mm";
        std::string areaUnit = "m²";
        std::string volumeUnit = "m³";
        std::string weightUnit = "kg";
        std::string temperatureUnit = "°C";
        
        // Conversion factors to base units (mm, mm², mm³, g, °C)
        std::unordered_map<std::string, double> conversionFactors;
    };
    
    /**
     * @brief Currency configuration
     */
    struct CurrencyConfig {
        std::string code = "USD";
        std::string symbol = "$";
        std::string name = "US Dollar";
        int decimalPlaces = 2;
        std::string decimalSeparator = ".";
        std::string thousandsSeparator = ",";
        bool symbolBefore = true;
    };
    
    /**
     * @brief Theme configuration
     */
    struct ThemeConfig {
        std::string name = "default";
        std::string displayName = "Default";
        bool isDark = false;
        
        // Colors (hex format)
        std::string primaryColor = "#2196F3";
        std::string secondaryColor = "#FFC107";
        std::string backgroundColor = "#FFFFFF";
        std::string textColor = "#000000";
        std::string accentColor = "#FF5722";
        
        // Fonts
        std::string fontFamily = "Arial";
        int fontSize = 12;
        
        // UI settings
        int borderRadius = 4;
        int spacing = 8;
        double opacity = 1.0;
    };

private:
    std::unordered_map<std::string, ConfigSection> sections_;
    std::string configFilePath_;
    
    // Cached configurations
    LanguageConfig languageConfig_;
    UnitConfig unitConfig_;
    CurrencyConfig currencyConfig_;
    ThemeConfig themeConfig_;
    
    // Available options
    std::vector<LanguageConfig> availableLanguages_;
    std::vector<CurrencyConfig> availableCurrencies_;
    std::vector<ThemeConfig> availableThemes_;
    
    // Callbacks
    std::function<void(const std::string&, const std::string&)> valueChangedCallback_;
    std::function<void(const std::string&)> languageChangedCallback_;
    std::function<void(const std::string&)> themeChangedCallback_;
    std::function<void(const std::string&)> errorCallback_;

public:
    /**
     * @brief Constructor
     */
    explicit ConfigurationService(const std::string& configFilePath = "config.json");
    
    /**
     * @brief Destructor - saves configuration
     */
    ~ConfigurationService();
    
    // Configuration management
    
    /**
     * @brief Load configuration from file
     */
    bool loadConfiguration();
    
    /**
     * @brief Save configuration to file
     */
    bool saveConfiguration() const;
    
    /**
     * @brief Reset to default configuration
     */
    void resetToDefaults();
    
    /**
     * @brief Import configuration from file
     */
    bool importConfiguration(const std::string& filePath);
    
    /**
     * @brief Export configuration to file
     */
    bool exportConfiguration(const std::string& filePath) const;
    
    // Section management
    
    /**
     * @brief Create or get configuration section
     */
    ConfigSection& getSection(const std::string& sectionName, const std::string& description = "");
    
    /**
     * @brief Check if section exists
     */
    bool hasSection(const std::string& sectionName) const;
    
    /**
     * @brief Remove section
     */
    void removeSection(const std::string& sectionName);
    
    /**
     * @brief Get all section names
     */
    std::vector<std::string> getSectionNames() const;
    
    // Value access (convenience methods)
    
    /**
     * @brief Set configuration value
     */
    template<typename T>
    void setValue(const std::string& section, const std::string& key, const T& value, 
                 const std::string& description = "", bool readOnly = false) {
        getSection(section).setValue(key, value, description, readOnly);
        notifyValueChanged(section, key);
    }
    
    /**
     * @brief Get configuration value
     */
    template<typename T>
    T getValue(const std::string& section, const std::string& key, const T& defaultValue = T{}) const {
        auto it = sections_.find(section);
        if (it != sections_.end()) {
            return it->second.getValue<T>(key, defaultValue);
        }
        return defaultValue;
    }
    
    /**
     * @brief Check if value exists
     */
    bool hasValue(const std::string& section, const std::string& key) const;
    
    /**
     * @brief Remove value
     */
    void removeValue(const std::string& section, const std::string& key);
    
    // Language and localization
    
    /**
     * @brief Set current language
     */
    void setLanguage(const std::string& languageCode);
    
    /**
     * @brief Get current language configuration
     */
    const LanguageConfig& getLanguageConfig() const { return languageConfig_; }
    
    /**
     * @brief Get available languages
     */
    const std::vector<LanguageConfig>& getAvailableLanguages() const { return availableLanguages_; }
    
    /**
     * @brief Add available language
     */
    void addAvailableLanguage(const LanguageConfig& language);
    
    /**
     * @brief Get localized string (placeholder for future i18n implementation)
     */
    std::string getLocalizedString(const std::string& key, const std::string& defaultValue = "") const;
    
    // Units and measurements
    
    /**
     * @brief Set unit system
     */
    void setUnitSystem(UnitConfig::System system);
    
    /**
     * @brief Get current unit configuration
     */
    const UnitConfig& getUnitConfig() const { return unitConfig_; }
    
    /**
     * @brief Convert value between units
     */
    double convertUnit(double value, const std::string& fromUnit, const std::string& toUnit) const;
    
    /**
     * @brief Format value with unit
     */
    std::string formatWithUnit(double value, const std::string& unit) const;
    
    // Currency
    
    /**
     * @brief Set current currency
     */
    void setCurrency(const std::string& currencyCode);
    
    /**
     * @brief Get current currency configuration
     */
    const CurrencyConfig& getCurrencyConfig() const { return currencyConfig_; }
    
    /**
     * @brief Get available currencies
     */
    const std::vector<CurrencyConfig>& getAvailableCurrencies() const { return availableCurrencies_; }
    
    /**
     * @brief Format currency value
     */
    std::string formatCurrency(double value) const;
    
    // Theme and UI
    
    /**
     * @brief Set current theme
     */
    void setTheme(const std::string& themeName);
    
    /**
     * @brief Get current theme configuration
     */
    const ThemeConfig& getThemeConfig() const { return themeConfig_; }
    
    /**
     * @brief Get available themes
     */
    const std::vector<ThemeConfig>& getAvailableThemes() const { return availableThemes_; }
    
    /**
     * @brief Add custom theme
     */
    void addCustomTheme(const ThemeConfig& theme);
    
    /**
     * @brief Remove custom theme
     */
    bool removeCustomTheme(const std::string& themeName);
    
    // Application-specific settings
    
    /**
     * @brief Get application settings section
     */
    ConfigSection& getApplicationSettings() { return getSection("application", "Application Settings"); }
    
    /**
     * @brief Get user interface settings section
     */
    ConfigSection& getUISettings() { return getSection("ui", "User Interface Settings"); }
    
    /**
     * @brief Get rendering settings section
     */
    ConfigSection& getRenderingSettings() { return getSection("rendering", "Rendering Settings"); }
    
    /**
     * @brief Get project settings section
     */
    ConfigSection& getProjectSettings() { return getSection("project", "Project Settings"); }
    
    /**
     * @brief Get export settings section
     */
    ConfigSection& getExportSettings() { return getSection("export", "Export Settings"); }
    
    // Validation
    
    /**
     * @brief Validate configuration
     */
    std::vector<std::string> validateConfiguration() const;
    
    /**
     * @brief Check if configuration is valid
     */
    bool isValid() const { return validateConfiguration().empty(); }
    
    // Callbacks
    
    /**
     * @brief Set callback for value changes
     */
    void setValueChangedCallback(std::function<void(const std::string&, const std::string&)> callback) {
        valueChangedCallback_ = callback;
    }
    
    /**
     * @brief Set callback for language changes
     */
    void setLanguageChangedCallback(std::function<void(const std::string&)> callback) {
        languageChangedCallback_ = callback;
    }
    
    /**
     * @brief Set callback for theme changes
     */
    void setThemeChangedCallback(std::function<void(const std::string&)> callback) {
        themeChangedCallback_ = callback;
    }
    
    /**
     * @brief Set callback for errors
     */
    void setErrorCallback(std::function<void(const std::string&)> callback) {
        errorCallback_ = callback;
    }
    
    // Utility methods
    
    /**
     * @brief Get configuration file path
     */
    const std::string& getConfigFilePath() const { return configFilePath_; }
    
    /**
     * @brief Set configuration file path
     */
    void setConfigFilePath(const std::string& filePath) { configFilePath_ = filePath; }
    
    /**
     * @brief Get configuration as JSON
     */
    nlohmann::json toJson() const;
    
    /**
     * @brief Load configuration from JSON
     */
    void fromJson(const nlohmann::json& json);
    
    /**
     * @brief Create backup of current configuration
     */
    bool createBackup(const std::string& backupPath = "") const;
    
    /**
     * @brief Restore configuration from backup
     */
    bool restoreFromBackup(const std::string& backupPath);

private:
    /**
     * @brief Initialize default configuration
     */
    void initializeDefaults();
    
    /**
     * @brief Initialize available languages
     */
    void initializeLanguages();
    
    /**
     * @brief Initialize available currencies
     */
    void initializeCurrencies();
    
    /**
     * @brief Initialize available themes
     */
    void initializeThemes();
    
    /**
     * @brief Initialize unit conversion factors
     */
    void initializeUnitConversions();
    
    /**
     * @brief Update cached configurations
     */
    void updateCachedConfigurations();
    
    /**
     * @brief Notify callbacks
     */
    void notifyValueChanged(const std::string& section, const std::string& key);
    void notifyLanguageChanged(const std::string& languageCode);
    void notifyThemeChanged(const std::string& themeName);
    void notifyError(const std::string& error) const;
    
    /**
     * @brief Find language by code
     */
    const LanguageConfig* findLanguage(const std::string& code) const;
    
    /**
     * @brief Find currency by code
     */
    const CurrencyConfig* findCurrency(const std::string& code) const;
    
    /**
     * @brief Find theme by name
     */
    const ThemeConfig* findTheme(const std::string& name) const;
};

} // namespace Services
} // namespace KitchenCAD