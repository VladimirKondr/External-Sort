/**
 * @file DefaultLogger.cpp
 * @brief Implementation of default logger factory function
 */

#include "ILogger.hpp"
#include "NullLogger.hpp"
#include <memory>

namespace logging {

std::shared_ptr<ILogger> CreateDefaultLogger() {
    return std::make_shared<NullLogger>();
}

}  // namespace logging
