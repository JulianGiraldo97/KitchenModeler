#pragma once

#include <string>
#include <fstream>
#include <memory>
#include <mutex>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <iostream>

namespace KitchenCAD {
namespace Utils {

/**
 * @brief Log levels for filtering messages
 */
enum class LogLevel {
    Debug = 0,
    Info = 1,
    Warning = 2,
    Error = 3,
    Critical = 4
};

/**
 * @brief Simple thread-safe logging system
 */
class Logger {
private:
    static std::unique_ptr<Logger> instance_;
    static std::mutex instanceMutex_;
    
    std::ofstream logFile_;
    LogLevel minLevel_;
    bool logToConsole_;
    bool logToFile_;
    std::mutex logMutex_;
    
    Logger() : minLevel_(LogLevel::Info), logToConsole_(true), logToFile_(false) {}
    
public:
    // Singleton access
    static Logger& getInstance() {
        std::lock_guard<std::mutex> lock(instanceMutex_);
        if (!instance_) {
            instance_ = std::unique_ptr<Logger>(new Logger());
        }
        return *instance_;
    }
    
    // Configuration
    void setLogLevel(LogLevel level) {
        std::lock_guard<std::mutex> lock(logMutex_);
        minLevel_ = level;
    }
    
    void setConsoleLogging(bool enabled) {
        std::lock_guard<std::mutex> lock(logMutex_);
        logToConsole_ = enabled;
    }
    
    void setFileLogging(const std::string& filename) {
        std::lock_guard<std::mutex> lock(logMutex_);
        
        if (logFile_.is_open()) {
            logFile_.close();
        }
        
        if (!filename.empty()) {
            logFile_.open(filename, std::ios::app);
            logToFile_ = logFile_.is_open();
        } else {
            logToFile_ = false;
        }
    }
    
    // Main logging method
    void log(LogLevel level, const std::string& message, const std::string& category = "") {
        if (level < minLevel_) {
            return;
        }
        
        std::lock_guard<std::mutex> lock(logMutex_);
        
        std::string formattedMessage = formatMessage(level, message, category);
        
        if (logToConsole_) {
            if (level >= LogLevel::Error) {
                std::cerr << formattedMessage << std::endl;
            } else {
                std::cout << formattedMessage << std::endl;
            }
        }
        
        if (logToFile_ && logFile_.is_open()) {
            logFile_ << formattedMessage << std::endl;
            logFile_.flush();
        }
    }
    
    // Convenience methods
    void debug(const std::string& message, const std::string& category = "") {
        log(LogLevel::Debug, message, category);
    }
    
    void info(const std::string& message, const std::string& category = "") {
        log(LogLevel::Info, message, category);
    }
    
    void warning(const std::string& message, const std::string& category = "") {
        log(LogLevel::Warning, message, category);
    }
    
    void error(const std::string& message, const std::string& category = "") {
        log(LogLevel::Error, message, category);
    }
    
    void critical(const std::string& message, const std::string& category = "") {
        log(LogLevel::Critical, message, category);
    }
    
    // Template method for formatted logging
    template<typename... Args>
    void logf(LogLevel level, const std::string& format, Args&&... args) {
        if (level < minLevel_) {
            return;
        }
        
        std::string message = formatString(format, std::forward<Args>(args)...);
        log(level, message);
    }
    
    template<typename... Args>
    void debugf(const std::string& format, Args&&... args) {
        logf(LogLevel::Debug, format, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void infof(const std::string& format, Args&&... args) {
        logf(LogLevel::Info, format, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void warningf(const std::string& format, Args&&... args) {
        logf(LogLevel::Warning, format, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void errorf(const std::string& format, Args&&... args) {
        logf(LogLevel::Error, format, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void criticalf(const std::string& format, Args&&... args) {
        logf(LogLevel::Critical, format, std::forward<Args>(args)...);
    }
    
    // Cleanup
    ~Logger() {
        if (logFile_.is_open()) {
            logFile_.close();
        }
    }
    
private:
    std::string formatMessage(LogLevel level, const std::string& message, const std::string& category) {
        std::ostringstream oss;
        
        // Timestamp
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        oss << "." << std::setfill('0') << std::setw(3) << ms.count();
        
        // Log level
        oss << " [" << logLevelToString(level) << "]";
        
        // Category
        if (!category.empty()) {
            oss << " [" << category << "]";
        }
        
        // Message
        oss << " " << message;
        
        return oss.str();
    }
    
    std::string logLevelToString(LogLevel level) {
        switch (level) {
            case LogLevel::Debug:    return "DEBUG";
            case LogLevel::Info:     return "INFO ";
            case LogLevel::Warning:  return "WARN ";
            case LogLevel::Error:    return "ERROR";
            case LogLevel::Critical: return "CRIT ";
            default:                 return "UNKN ";
        }
    }
    
    template<typename... Args>
    std::string formatString(const std::string& format, Args&&... args) {
        // Simple string formatting - in production you might want to use fmt library
        std::ostringstream oss;
        formatStringImpl(oss, format, std::forward<Args>(args)...);
        return oss.str();
    }
    
    void formatStringImpl(std::ostringstream& oss, const std::string& format) {
        oss << format;
    }
    
    template<typename T, typename... Args>
    void formatStringImpl(std::ostringstream& oss, const std::string& format, T&& value, Args&&... args) {
        size_t pos = format.find("{}");
        if (pos != std::string::npos) {
            oss << format.substr(0, pos) << value;
            formatStringImpl(oss, format.substr(pos + 2), std::forward<Args>(args)...);
        } else {
            oss << format;
        }
    }
};

// Static member definitions will be in the implementation file

} // namespace Utils
} // namespace KitchenCAD

// Convenience macros for logging
#define LOG_DEBUG(msg) KitchenCAD::Utils::Logger::getInstance().debug(msg)
#define LOG_INFO(msg) KitchenCAD::Utils::Logger::getInstance().info(msg)
#define LOG_WARNING(msg) KitchenCAD::Utils::Logger::getInstance().warning(msg)
#define LOG_ERROR(msg) KitchenCAD::Utils::Logger::getInstance().error(msg)
#define LOG_CRITICAL(msg) KitchenCAD::Utils::Logger::getInstance().critical(msg)

#define LOG_DEBUG_CAT(msg, cat) KitchenCAD::Utils::Logger::getInstance().debug(msg, cat)
#define LOG_INFO_CAT(msg, cat) KitchenCAD::Utils::Logger::getInstance().info(msg, cat)
#define LOG_WARNING_CAT(msg, cat) KitchenCAD::Utils::Logger::getInstance().warning(msg, cat)
#define LOG_ERROR_CAT(msg, cat) KitchenCAD::Utils::Logger::getInstance().error(msg, cat)
#define LOG_CRITICAL_CAT(msg, cat) KitchenCAD::Utils::Logger::getInstance().critical(msg, cat)

#define LOG_DEBUGF(fmt, ...) KitchenCAD::Utils::Logger::getInstance().debugf(fmt, __VA_ARGS__)
#define LOG_INFOF(fmt, ...) KitchenCAD::Utils::Logger::getInstance().infof(fmt, __VA_ARGS__)
#define LOG_WARNINGF(fmt, ...) KitchenCAD::Utils::Logger::getInstance().warningf(fmt, __VA_ARGS__)
#define LOG_ERRORF(fmt, ...) KitchenCAD::Utils::Logger::getInstance().errorf(fmt, __VA_ARGS__)
#define LOG_CRITICALF(fmt, ...) KitchenCAD::Utils::Logger::getInstance().criticalf(fmt, __VA_ARGS__)