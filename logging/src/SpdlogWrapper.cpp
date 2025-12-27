/**
 * @file SpdlogWrapper.cpp
 * @brief Implementation of the spdlog wrapper
 */

#include "SpdlogWrapper.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
#include <vector>
#include <memory>
#include <atomic>

namespace logging {

namespace {
// Counter for unique logger names
std::atomic<int> logger_counter{0};
}  // namespace

/**
 * @brief Implementation class hiding spdlog dependencies
 *
 * This nested class contains the actual spdlog logger instance,
 * keeping spdlog headers out of the public interface.
 */
class SpdlogWrapper::Impl {
public:
    Impl() = delete;

    /**
     * @brief Construct with specified sink configuration
     * @param sink_type Type of sink to create
     * @param filename Filename for file sink (if applicable)
     */
    Impl(const std::string& name, SpdlogSinkType sink_type, const std::string& filename) {
        std::vector<spdlog::sink_ptr> sinks;

        switch (sink_type) {
            case SpdlogSinkType::Console: {
                auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
                sinks.push_back(console_sink);
                break;
            }
            case SpdlogSinkType::File: {
                if (filename.empty()) {
                    throw std::invalid_argument("Filename required for File sink type");
                }
                auto file_sink =
                    std::make_shared<spdlog::sinks::basic_file_sink_mt>(filename, true);
                sinks.push_back(file_sink);
                break;
            }
            case SpdlogSinkType::Both: {
                if (filename.empty()) {
                    throw std::invalid_argument("Filename required for Both sink type");
                }
                auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
                auto file_sink =
                    std::make_shared<spdlog::sinks::basic_file_sink_mt>(filename, true);
                sinks.push_back(console_sink);
                sinks.push_back(file_sink);
                break;
            }
        }

        logger = std::make_shared<spdlog::logger>(name + "_" + std::to_string(logger_counter++),
                                                  sinks.begin(), sinks.end());
        logger->set_level(spdlog::level::trace);
    }

    std::shared_ptr<spdlog::logger> logger;  ///< The underlying spdlog logger
};

SpdlogWrapper::SpdlogWrapper(const std::string& name, SpdlogSinkType sink_type,
                             const std::string& filename)
    : pimpl_(std::make_unique<Impl>(name, sink_type, filename)) {
}

SpdlogWrapper::~SpdlogWrapper() = default;

SpdlogWrapper::SpdlogWrapper(SpdlogWrapper&&) noexcept = default;

SpdlogWrapper& SpdlogWrapper::operator=(SpdlogWrapper&&) noexcept = default;

void SpdlogWrapper::LogInfo(const std::string& message) {
    pimpl_->logger->info(message);
}

void SpdlogWrapper::LogWarning(const std::string& message) {
    pimpl_->logger->warn(message);
}

void SpdlogWrapper::LogError(const std::string& message) {
    pimpl_->logger->error(message);
}

}  // namespace logging
