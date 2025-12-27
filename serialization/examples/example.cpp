/**
 * @file example_spdlog.cpp
 * @brief Example demonstrating serialization library with different logger backends
 *
 * This example shows the serialization library in action with various data types.
 * The logger can be easily swapped by changing the initialization code at the top of main().
 *
 * Features demonstrated:
 * - Simple string serialization
 * - Vector of strings with Unicode/emoji support
 * - Nested vectors (vector of vectors)
 * - Error handling and logging
 */

#include "../include/serializers.hpp"
#include "../../logging/include/Registry.hpp"
#include "../../logging/include/SpdlogWrapper.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <cstdio>
#include <fstream>

using serialization::CreateSerializer;

/**
 * @brief Custom console logger with colored output and message prefixes
 */
class ConsoleLogger {
public:
    void info(const std::string& msg) {  // NOLINT(readability-identifier-naming)
        std::cout << "\033[32m[INFO]\033[0m " << msg << std::endl;
    }

    void warn(const std::string& msg) {  // NOLINT(readability-identifier-naming)
        std::cout << "\033[33m[WARN]\033[0m " << msg << std::endl;
    }

    void error(const std::string& msg) {  // NOLINT(readability-identifier-naming)
        std::cerr << "\033[31m[ERROR]\033[0m " << msg << std::endl;
    }
};

/**
 * @brief Non-copyable file logger that writes to a log file
 *
 * This logger uses RAII to manage a file handle and is explicitly non-copyable
 * to demonstrate that LoggerAdapter can handle such types via shared_ptr.
 */
class FileLogger {
public:
    explicit FileLogger(const std::string& filename) : log_file_(filename, std::ios::app) {
        if (log_file_.is_open()) {
            log_file_ << "\n=== New logging session started ===\n";
        }
    }

    ~FileLogger() {
        if (log_file_.is_open()) {
            log_file_ << "=== Logging session ended ===\n\n";
        }
    }

    FileLogger(const FileLogger&) = delete;
    FileLogger& operator=(const FileLogger&) = delete;
    FileLogger(FileLogger&&) = default;
    FileLogger& operator=(FileLogger&&) = default;

    void info(const std::string& msg) {  // NOLINT(readability-identifier-naming)
        if (log_file_.is_open()) {
            log_file_ << "[INFO] " << msg << std::endl;
        }
    }

    void warn(const std::string& msg) {  // NOLINT(readability-identifier-naming)
        if (log_file_.is_open()) {
            log_file_ << "[WARN] " << msg << std::endl;
        }
    }

    void error(const std::string& msg) {  // NOLINT(readability-identifier-naming)
        if (log_file_.is_open()) {
            log_file_ << "[ERROR] " << msg << std::endl;
        }
    }

private:
    std::ofstream log_file_;
};

int main() {
    // ============================================================================
    // LOGGER SETUP - Change this section to use a different logger
    // ============================================================================

    // spdlog
    auto spdlog_logger = std::make_shared<logging::SpdlogWrapper>(
        "spdlog", logging::SpdlogSinkType::Both, "logs.log");
    logging::SetLogger(spdlog_logger);

    // NullLogger
    // logging::SetDefaultLogger();

    // Custom logger
    // logging::SetLogger(ConsoleLogger{});

    // Custom non-copyable logger
    // auto file_logger = std::make_shared<FileLogger>(log_filename);
    // logging::SetLogger(file_logger);
    // ============================================================================

    // Get logger instance for application logging
    auto& logger = logging::detail::GetLoggerInstance();

    logger->LogInfo("=== Serialization Library Example ===");
    logger->LogInfo("Demonstrating various serialization scenarios");

    const char* filename = "serialization_example.bin";

    // Example 1: Basic string serialization
    logger->LogInfo("");
    logger->LogInfo("Example 1: std::string serialization");
    {
        std::string original = "Hello, Serialization!";
        logger->LogInfo("Original string: \"" + original + "\"");

        auto serializer = CreateSerializer<std::string>();

        // Serialize
        FILE* file = fopen(filename, "wb");
        if (file) {
            logger->LogInfo("Serializing...");
            bool success = serializer->Serialize(original, file);
            fclose(file);
            if (success) {
                logger->LogInfo("Serialization: SUCCESS");
            } else {
                logger->LogError("Serialization: FAILED");
            }
        }

        // Deserialize
        std::string loaded;
        file = fopen(filename, "rb");
        if (file) {
            logger->LogInfo("Deserializing...");
            bool success = serializer->Deserialize(loaded, file);
            fclose(file);
            if (success) {
                logger->LogInfo("Deserialization: SUCCESS");
                logger->LogInfo("Loaded string: \"" + loaded + "\"");

                // Verification
                if (original == loaded) {
                    logger->LogInfo("Verification passed: strings match");
                } else {
                    logger->LogError("Verification failed: strings do NOT match");
                }
            } else {
                logger->LogError("Deserialization: FAILED");
            }
        }
    }

    // Example 2: Vector of strings
    logger->LogInfo("");
    logger->LogInfo("Example 2: std::vector<std::string> serialization");
    {
        std::vector<std::string> original = {"First item", "Second item",
                                             "Third item with Unicode: αβγδ",
                                             "Fourth item with emoji 🚀"};

        logger->LogInfo("Original vector (" + std::to_string(original.size()) + " elements):");
        for (size_t i = 0; i < original.size(); ++i) {
            logger->LogInfo("  [" + std::to_string(i) + "] \"" + original[i] + "\"");
        }

        auto serializer = CreateSerializer<std::vector<std::string>>();

        // Serialize
        FILE* file = fopen(filename, "wb");
        if (file) {
            logger->LogInfo("Serializing...");
            bool success = serializer->Serialize(original, file);
            fclose(file);
            if (success) {
                logger->LogInfo("Serialization: SUCCESS");
            } else {
                logger->LogError("Serialization: FAILED");
            }
        }

        // Deserialize
        std::vector<std::string> loaded;
        file = fopen(filename, "rb");
        if (file) {
            logger->LogInfo("Deserializing...");
            bool success = serializer->Deserialize(loaded, file);
            fclose(file);
            if (success) {
                logger->LogInfo("Deserialization: SUCCESS");
                logger->LogInfo("Loaded vector (" + std::to_string(loaded.size()) + " elements):");
                for (size_t i = 0; i < loaded.size(); ++i) {
                    logger->LogInfo("  [" + std::to_string(i) + "] \"" + loaded[i] + "\"");
                }

                // Verification
                if (original == loaded) {
                    logger->LogInfo("Verification passed: vectors match");
                } else {
                    logger->LogError("Verification failed: vectors do NOT match");
                }
            } else {
                logger->LogError("Deserialization: FAILED");
            }
        }
    }

    // Example 3: Nested vectors
    logger->LogInfo("");
    logger->LogInfo("Example 3: std::vector<std::vector<int>> serialization");
    {
        std::vector<std::vector<int>> original = {{1, 2, 3}, {4, 5, 6, 7}, {8, 9}};

        logger->LogInfo("Original nested vector:");
        for (size_t i = 0; i < original.size(); ++i) {
            std::string row = "  Row " + std::to_string(i) + ": [";
            for (size_t j = 0; j < original[i].size(); ++j) {
                row += std::to_string(original[i][j]);
                if (j < original[i].size() - 1) {
                    row += ", ";
                }
            }
            row += "]";
            logger->LogInfo(row);
        }

        auto serializer = CreateSerializer<std::vector<std::vector<int>>>();

        // Serialize
        FILE* file = fopen(filename, "wb");
        if (file) {
            logger->LogInfo("Serializing...");
            bool success = serializer->Serialize(original, file);
            fclose(file);
            if (success) {
                logger->LogInfo("Serialization: SUCCESS");
            } else {
                logger->LogError("Serialization: FAILED");
            }
        }

        // Deserialize
        std::vector<std::vector<int>> loaded;
        file = fopen(filename, "rb");
        if (file) {
            logger->LogInfo("Deserializing...");
            bool success = serializer->Deserialize(loaded, file);
            fclose(file);
            if (success) {
                logger->LogInfo("Deserialization: SUCCESS");
                logger->LogInfo("Loaded nested vector:");
                for (size_t i = 0; i < loaded.size(); ++i) {
                    std::string row = "  Row " + std::to_string(i) + ": [";
                    for (size_t j = 0; j < loaded[i].size(); ++j) {
                        row += std::to_string(loaded[i][j]);
                        if (j < loaded[i].size() - 1) {
                            row += ", ";
                        }
                    }
                    row += "]";
                    logger->LogInfo(row);
                }

                // Verification
                if (original == loaded) {
                    logger->LogInfo("Verification passed: nested vectors match");
                } else {
                    logger->LogError("Verification failed: nested vectors do NOT match");
                }
            } else {
                logger->LogError("Deserialization: FAILED");
            }
        }
    }

    // Example 4: Error handling demonstration
    logger->LogInfo("");
    logger->LogInfo("Example 4: Error handling");
    {
        struct TestData {
            int x, y, z;
        };

        auto serializer = CreateSerializer<TestData>();

        logger->LogInfo("Creating empty file to trigger error...");
        FILE* file = fopen(filename, "wb");
        if (file) {
            fclose(file);
        }

        TestData data{0, 0, 0};
        file = fopen(filename, "rb");
        if (file) {
            logger->LogInfo("Attempting to deserialize from empty file (will fail)...");
            bool success = serializer->Deserialize(data, file);
            fclose(file);
            if (!success) {
                logger->LogWarning("Deserialization failed as expected");
                logger->LogWarning("See error message above from serialization library");
            }
        }
    }

    // Cleanup and summary
    logger->LogInfo("");
    logger->LogInfo("=== Example completed ===");
    logger->LogInfo("All serialization operations were logged");
    logger->LogInfo("Summary: 3 successful examples + 1 error handling demo");

    std::remove(filename);
    return 0;
}
