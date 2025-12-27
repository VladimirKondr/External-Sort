/**
 * @file ILogger.hpp
 * @brief Interface definition for all logger implementations
 */

#pragma once

#include <memory>
#include <string>

namespace logging {

/**
 * @brief Abstract base class defining the logger interface
 *
 * All logger implementations must inherit from this interface and implement
 * the three logging methods for different severity levels: info, warning, and error.
 * This allows the library to work with any logging backend through polymorphism.
 */
class ILogger {
public:
    virtual ~ILogger() = default;

    /**
     * @brief Log an informational message
     * @param message The message to log
     */
    virtual void LogInfo(const std::string& message) = 0;

    /**
     * @brief Log a warning message
     * @param message The message to log
     */
    virtual void LogWarning(const std::string& message) = 0;

    /**
     * @brief Log an error message
     * @param message The message to log
     */
    virtual void LogError(const std::string& message) = 0;
};

/**
 * @brief Factory function to create a default logger instance
 * @return Shared pointer to a NullLogger (default no-op logger)
 */
std::shared_ptr<ILogger> CreateDefaultLogger();

}  // namespace logging
