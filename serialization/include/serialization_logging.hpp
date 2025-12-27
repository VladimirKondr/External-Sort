/**
 * @file serialization_logging.hpp
 * @brief Logging integration for serialization library
 */

#pragma once

#include "../../logging/include/Registry.hpp"
#include <string>

namespace serialization {
namespace detail {

/**
 * @brief Log an error message from serialization operations
 * @param message The error message to log
 */
inline void LogError(const std::string& message) {
    auto& logger = logging::detail::GetLoggerInstance();
    if (logger) {
        logger->LogError("[Serialization] " + message);
    }
}

/**
 * @brief Log a warning message from serialization operations
 * @param message The warning message to log
 */
inline void LogWarning(const std::string& message) {
    auto& logger = logging::detail::GetLoggerInstance();
    if (logger) {
        logger->LogWarning("[Serialization] " + message);
    }
}

/**
 * @brief Log an informational message from serialization operations
 * @param message The info message to log
 */
inline void LogInfo(const std::string& message) {
    auto& logger = logging::detail::GetLoggerInstance();
    if (logger) {
        logger->LogInfo("[Serialization] " + message);
    }
}

}  // namespace detail
}  // namespace serialization
