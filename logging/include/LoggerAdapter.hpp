/**
 * @file LoggerAdapter.hpp
 * @brief Wrapper for any user-defined logger to be used with the logging library
 */

#pragma once

#include "ILogger.hpp"
#include <concepts>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

namespace logging {

/**
 * @brief Adapter class that wraps a user-defined logger to conform to the ILogger interface.
 *
 * This template class allows any custom logger implementation to be used with current library by
 * adapting its interface to match the ILogger contract. The user logger must provide info(),
 * warn(), and error() methods that accept string messages.
 *
 * The adapter automatically handles both copyable and non-copyable loggers:
 * - Copyable loggers are stored by value
 * - Non-copyable loggers are stored via shared_ptr
 *
 * @tparam UserLogger The type of the user-defined logger to be adapted
 *
 * Example usage with copyable logger:
 * @code
 * struct MyLogger {
 *     void info(const std::string& msg) { std::cout << "[INFO] " << msg << '\n'; }
 *     void warn(const std::string& msg) { std::cout << "[WARN] " << msg << '\n'; }
 *     void error(const std::string& msg) { std::cerr << "[ERROR] " << msg << '\n'; }
 * };
 *
 * auto adapter = std::make_shared<LoggerAdapter<MyLogger>>(MyLogger{});
 * @endcode
 *
 * Example usage with non-copyable logger stored via shared_ptr:
 * @code
 * struct HeavyLogger {
 *     HeavyLogger() = default;
 *     HeavyLogger(const HeavyLogger&) = delete;
 *     void info(const std::string& msg) { }
 *     void warn(const std::string& msg) { }
 *     void error(const std::string& msg) { }
 * };
 *
 * auto adapter = std::make_shared<LoggerAdapter<HeavyLogger>>(
 *     std::make_shared<HeavyLogger>());
 * @endcode
 */
template <typename UserLogger>
class LoggerAdapter : public ILogger {
public:
    /**
     * @brief Construct adapter for copyable logger by forwarding the user logger
     * @tparam T Deduced type for perfect forwarding
     * @param logger The user-defined logger instance to wrap (by value)
     */
    template <typename T>
        requires std::copy_constructible<std::decay_t<T>> &&
                 (!std::same_as<std::decay_t<T>, std::shared_ptr<UserLogger>>)
    explicit LoggerAdapter(T&& logger) : user_logger_(std::forward<T>(logger)) {
    }

    /**
     * @brief Construct adapter for non-copyable logger via shared_ptr
     * @param logger Shared pointer to the user-defined logger instance
     */
    explicit LoggerAdapter(std::shared_ptr<UserLogger> logger)
        : user_logger_ptr_(std::move(logger)) {
    }

    /**
     * @brief Log an informational message using the wrapped logger
     * @param message The message to log
     */
    void LogInfo(const std::string& message) override {
        if (user_logger_ptr_) {
            user_logger_ptr_->info(message);
        } else {
            user_logger_->info(message);
        }
    }

    /**
     * @brief Log a warning message using the wrapped logger
     * @param message The message to log
     */
    void LogWarning(const std::string& message) override {
        if (user_logger_ptr_) {
            user_logger_ptr_->warn(message);
        } else {
            user_logger_->warn(message);
        }
    }

    /**
     * @brief Log an error message using the wrapped logger
     * @param message The message to log
     */
    void LogError(const std::string& message) override {
        if (user_logger_ptr_) {
            user_logger_ptr_->error(message);
        } else {
            user_logger_->error(message);
        }
    }

private:
    std::optional<UserLogger> user_logger_;        ///< Logger stored by value (for copyable types)
    std::shared_ptr<UserLogger> user_logger_ptr_;  ///< Logger stored by pointer (for non-copyable)
};

}  // namespace logging
