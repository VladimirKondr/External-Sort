/**
 * @file SpdlogWrapper.hpp
 * @brief Wrapper for spdlog library using PIMPL pattern
 */

#pragma once

#include "ILogger.hpp"
#include <memory>
#include <string>

namespace logging {

/**
 * @brief Types of logging sinks available for SpdlogWrapper
 */
enum class SpdlogSinkType {
    Console,  ///< Log to console (stdout) with colors
    File,     ///< Log to file only
    Both      ///< Log to both console and file
};

/**
 * @brief Concrete logger implementation wrapping the spdlog library
 *
 * Uses the PIMPL idiom to hide spdlog dependencies
 * from the header, reducing compilation dependencies and allowing the logging
 * library to be used without requiring spdlog to be installed.
 *
 * Supports multiple output targets:
 * - Console only (default)
 * - File only
 * - Both console and file simultaneously
 */
class SpdlogWrapper : public ILogger {
public:
    /**
     * @brief Construct with specified sink type
     * @param sink_type Type of sink to use (Console, File, or Both)
     * @param filename Filename for file sink (required if sink_type is File or Both)
     *
     * Example:
     * @code
     * // Console only
     * auto console_logger = std::make_shared<SpdlogWrapper>(SpdlogSinkType::Console);
     *
     * // File only
     * auto file_logger = std::make_shared<SpdlogWrapper>(SpdlogSinkType::File, "app.log");
     *
     * // Both console and file
     * auto both_logger = std::make_shared<SpdlogWrapper>(SpdlogSinkType::Both, "app.log");
     * @endcode
     */
    explicit SpdlogWrapper(const std::string& name = "spdlog",
                           SpdlogSinkType sink_type = SpdlogSinkType::Console,
                           const std::string& filename = "logs.log");

    ~SpdlogWrapper();
    SpdlogWrapper(SpdlogWrapper&&) noexcept;
    SpdlogWrapper& operator=(SpdlogWrapper&&) noexcept;
    SpdlogWrapper(const SpdlogWrapper&) = delete;
    SpdlogWrapper& operator=(const SpdlogWrapper&) = delete;

    /**
     * @brief Log an informational message
     * @param message The message to log
     */
    void LogInfo(const std::string& message) override;

    /**
     * @brief Log a warning message
     * @param message The message to log
     */
    void LogWarning(const std::string& message) override;

    /**
     * @brief Log an error message
     * @param message The message to log
     */
    void LogError(const std::string& message) override;

private:
    class Impl;                    ///< Forward declaration of implementation class
    std::unique_ptr<Impl> pimpl_;  ///< Pointer to implementation
};

}  // namespace logging
