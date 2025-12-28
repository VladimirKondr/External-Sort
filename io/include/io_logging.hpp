/**
 * @file io_logging.hpp
 * @brief Logging integration for io library
 */

#pragma once

#include "Registry.hpp"
#include <string>

namespace io {
namespace detail {

/**
 * @brief Log an error message from io operations
 * @param message The error message to log
 */
inline void LogError(const std::string& message) {
    auto& logger = logging::detail::GetLoggerInstance();
    if (logger) {
        logger->LogError("[IO] " + message);
    }
}

/**
 * @brief Log a warning message from io operations
 * @param message The warning message to log
 */
inline void LogWarning(const std::string& message) {
    auto& logger = logging::detail::GetLoggerInstance();
    if (logger) {
        logger->LogWarning("[IO] " + message);
    }
}

/**
 * @brief Log an informational message from io operations
 * @param message The info message to log
 */
inline void LogInfo(const std::string& message) {
    auto& logger = logging::detail::GetLoggerInstance();
    if (logger) {
        logger->LogInfo("[IO] " + message);
    }
}

}  // namespace detail
}  // namespace io
