/**
 * @file NullLogger.hpp
 * @brief Empty no-op logger implementation for when logging is disabled
 */

#pragma once

#include <string>
#include "ILogger.hpp"

namespace logging {

/**
 * @brief A logger implementation that discards all log messages
 *
 * This is the default logger used when no custom logger is set.
 * All logging methods are no-ops, making it suitable for production
 * code where logging overhead should be minimal or when logging
 * is not desired.
 */
class NullLogger : public ILogger {
public:
    /**
     * @brief Log an informational message (no-op)
     * @param message The message to discard (unused)
     */
    void LogInfo(const std::string& message) override {
        (void)message;  // Suppress unused parameter warning
    }

    /**
     * @brief Log a warning message (no-op)
     * @param message The message to discard (unused)
     */
    void LogWarning(const std::string& message) override {
        (void)message;  // Suppress unused parameter warning
    }

    /**
     * @brief Log an error message (no-op)
     * @param message The message to discard (unused)
     */
    void LogError(const std::string& message) override {
        (void)message;  // Suppress unused parameter warning
    }
};

}  // namespace logging
