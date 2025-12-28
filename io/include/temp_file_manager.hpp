/**
 * @file temp_file_manager.hpp
 * @brief Temporary file manager
 */

#pragma once

#include "io_logging.hpp"

#include <cstdint>
#include <filesystem>
#include <string>

namespace io {

/**
 * @brief A manager for temporary files
 *
 * The class is responsible for creating, tracking, and deleting temporary files
 * used during the external sorting process.
 */
class TempFileManager {
private:
    std::filesystem::path base_temp_dir_path_;  ///< Base directory for temporary files
    uint64_t temp_file_counter_ = 0;            ///< Counter for generating unique names
    bool owns_directory_{};  ///< Flag to indicate if this instance owns the directory

public:
    /**
     * @brief Constructs a new TempFileManager object
     * @param base_dir_name The name of the base directory for temporary files
     */
    explicit TempFileManager(const std::string& base_dir_name = "ts");

    /**
     * @brief Destructor that cleans up all temporary files
     */
    ~TempFileManager();

    TempFileManager(const TempFileManager&) = delete;
    TempFileManager& operator=(const TempFileManager&) = delete;

    TempFileManager(TempFileManager&&) = default;
    TempFileManager& operator=(TempFileManager&&) = default;

    /**
     * @brief Generates a unique temporary filename
     * @param prefix The prefix for the filename
     * @param extension The file extension
     * @return The full path to the temporary file
     */
    std::string GenerateTempFilename(const std::string& prefix = "tmp",
                                     const std::string& extension = ".b");

    /**
     * @brief Deletes the specified file
     * @param filename_str Path to the file to be deleted
     */
    void CleanupFile(const std::string& filename_str);

    /**
     * @brief Returns the path to the base directory
     * @return A const reference to the base directory path
     */
    const std::filesystem::path& GetBaseDirPath() const;
};

TempFileManager::TempFileManager(const std::string& base_dir_name) {
    base_temp_dir_path_ = std::filesystem::current_path() / base_dir_name;
    if (!std::filesystem::exists(base_temp_dir_path_)) {
        std::error_code ec;
        std::filesystem::create_directories(base_temp_dir_path_, ec);
        if (ec) {
            throw std::runtime_error("TempFileManager failed to create temp directory: " +
                                     base_temp_dir_path_.string() + " Error: " + ec.message());
        }
        owns_directory_ = true;
        detail::LogInfo("TempFileManager created temporary directory: " + base_temp_dir_path_.string());
    } else {
        owns_directory_ = false;
        detail::LogInfo("TempFileManager using existing temporary directory: "
                        + base_temp_dir_path_.string());
    }
}

TempFileManager::~TempFileManager() {
    detail::LogInfo("TempFileManager destructor: Attempting to clean up temporary directory...");
    if (owns_directory_ && std::filesystem::exists(base_temp_dir_path_)) {
        std::error_code ec;
        std::filesystem::remove_all(base_temp_dir_path_, ec);
        if (ec) {
            detail::LogWarning("Warning: TempFileManager failed to remove_all temp directory "
                               + base_temp_dir_path_.string() + ": " + ec.message());
        } else {
            detail::LogInfo("TempFileManager removed temp directory and all its contents: "
                            + base_temp_dir_path_.string());
        }
    } else if (std::filesystem::exists(base_temp_dir_path_)) {
        detail::LogInfo("TempFileManager: Temporary directory "
                        + base_temp_dir_path_.string()
                        + " exists but was not created by this instance, not removing.");
    }
}

std::string TempFileManager::GenerateTempFilename(const std::string& prefix,
                                                  const std::string& extension) {
    std::filesystem::path file_path =
        base_temp_dir_path_ / (prefix + std::to_string(temp_file_counter_++) + extension);
    detail::LogInfo("TempFileManager generated temp filename: " + file_path.string());
    return file_path.string();
}

void TempFileManager::CleanupFile(const std::string& filename_str) {
    std::filesystem::path file_path(filename_str);
    if (std::filesystem::exists(file_path)) {
        std::error_code ec;
        std::filesystem::remove(file_path, ec);
        if (ec) {
            detail::LogWarning("CleanupFile failed to remove " + filename_str + ": "
                               + ec.message());
        }
    }
}

const std::filesystem::path& TempFileManager::GetBaseDirPath() const {
    return base_temp_dir_path_;
}

}  // namespace io
