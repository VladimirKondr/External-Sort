/**
 * @file io_logging.hpp
 * @brief Logging integration for External Sort library
 */

#pragma once

#include "Registry.hpp"
#include <string>

namespace external_sort {
namespace detail {

/**
 * @brief Log an error message from External Sort operations
 * @param message The error message External Sort log
 */
inline void LogError(const std::string& message) {
    auto& logger = logging::detail::GetLoggerInstance();
    if (logger) {
        logger->LogError("[External Sort] " + message);
    }
}

/**
 * @brief Log a warning message from io operations
 * @param message The warning message External Sort log
 */
inline void LogWarning(const std::string& message) {
    auto& logger = logging::detail::GetLoggerInstance();
    if (logger) {
        logger->LogWarning("[External Sort] " + message);
    }
}

/**
 * @brief Log an informational message from External Sort operations
 * @param message The info message to log
 */
inline void LogInfo(const std::string& message) {
    auto& logger = logging::detail::GetLoggerInstance();
    if (logger) {
        logger->LogInfo("[External Sort] " + message);
    }
}

}  // namespace detail
}  // namespace external_sort
