/**
 * @file Registry.hpp
 * @brief Global logger registry for setting and managing the active logger instance
 */

#pragma once

#include "ILogger.hpp"
#include "NullLogger.hpp"
#include "LoggerAdapter.hpp"
#include <memory>
#include <mutex>
#include <utility>

namespace logging {

namespace detail {
/**
 * @brief Get the global logger instance
 * @return Reference to the shared pointer holding the current logger
 */
inline std::shared_ptr<ILogger>& GetLoggerInstance() {
    static std::shared_ptr<ILogger> logger = std::make_shared<NullLogger>();
    return logger;
}

/**
 * @brief Get the mutex protecting the logger instance
 * @return Reference to the global logger mutex
 */
inline std::mutex& GetLoggerMutex() {
    static std::mutex m;
    return m;
}
}  // namespace detail

/**
 * @brief Set a custom user-defined logger as the global logger
 *
 * Wraps the user logger in a LoggerAdapter and sets it as the active logger.
 * The user logger must provide info(), warn(), and error() methods.
 * Thread-safe: uses a mutex to protect the global logger instance.
 *
 * @tparam UserLogger The type of the user-defined logger
 * @param user_logger The logger instance to set (will be moved or copied)
 *
 * Example:
 * @code
 * struct MyLogger {
 *     void info(const std::string& msg) { }
 *     void warn(const std::string& msg) { }
 *     void error(const std::string& msg) { }
 * };
 * SetLogger(MyLogger{});
 * @endcode
 */
template <typename UserLogger>
void SetLogger(UserLogger&& user_logger) {
    auto adapter = std::make_shared<LoggerAdapter<std::decay_t<UserLogger>>>(
        std::forward<UserLogger>(user_logger));
    std::lock_guard<std::mutex> lock(detail::GetLoggerMutex());
    detail::GetLoggerInstance() = std::move(adapter);
}

/**
 * @brief Set a custom user-defined logger passed in std::shared_ptr as the global logger
 *
 * Wraps the user logger (held by shared_ptr) in a LoggerAdapter and sets it as the active logger.
 * This overload can be used for non-copyable user loggers.
 * Thread-safe: uses a mutex to protect the global logger instance.
 *
 * @tparam UserLogger The type of the user-defined logger (not derived from ILogger)
 * @param user_logger Shared pointer to the user logger instance
 *
 * Example:
 * @code
 * struct HeavyLogger {
 *     HeavyLogger() = default;
 *     HeavyLogger(const HeavyLogger&) = delete;
 *     void info(const std::string& msg) { }
 *     void warn(const std::string& msg) { }
 *     void error(const std::string& msg) { }
 * };
 * auto logger = std::make_shared<HeavyLogger>();
 * SetLogger(logger);
 * @endcode
 */
template <typename UserLogger>
    requires (!std::derived_from<UserLogger, ILogger>)
void SetLogger(std::shared_ptr<UserLogger> user_logger) {
    auto adapter = std::make_shared<LoggerAdapter<UserLogger>>(std::move(user_logger));
    std::lock_guard<std::mutex> lock(detail::GetLoggerMutex());
    detail::GetLoggerInstance() = std::move(adapter);
}

/**
 * @brief Set a logger instance derived from ILogger as the global logger
 *
 * Thread-safe: uses a mutex to protect the global logger instance.
 * This overload is used when passing a shared pointer to a type that derives from ILogger.
 *
 * @tparam DerivedLogger A type that derives from ILogger
 * @param logger Shared pointer to the derived logger instance to set
 */
template <typename DerivedLogger>
    requires std::derived_from<DerivedLogger, ILogger>
void SetLogger(std::shared_ptr<DerivedLogger> logger) {
    std::lock_guard<std::mutex> lock(detail::GetLoggerMutex());
    detail::GetLoggerInstance() = std::move(logger);
}

/**
 * @brief Set a logger instance that already conforms to the ILogger interface
 *
 * Thread-safe: uses a mutex to protect the global logger instance.
 * If nullptr is passed, sets a NullLogger instead.
 *
 * @param logger Shared pointer to the logger to set, or nullptr for NullLogger
 */
inline void SetLogger(std::shared_ptr<ILogger> logger) {
    std::lock_guard<std::mutex> lock(detail::GetLoggerMutex());
    detail::GetLoggerInstance() = logger ? std::move(logger) : std::make_shared<NullLogger>();
}

/**
 * @brief Reset the logger to the default NullLogger
 * 
 * Convenience function that sets a NullLogger instance.
 * Thread-safe: uses a mutex to protect the global logger instance.
 */
inline void SetDefaultLogger() {
    std::lock_guard<std::mutex> lock(detail::GetLoggerMutex());
    detail::GetLoggerInstance() = std::make_shared<NullLogger>();
}



}  // namespace logging
