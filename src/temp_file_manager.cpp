/**
 * @file temp_file_manager.cpp
 * @brief Реализация менеджера временных файлов
 * @author External Sort Library
 * @version 1.0
 */

#include "temp_file_manager.hpp"

#include "../include/debug_logger.hpp"
#include "storage_types.hpp"

#include <iostream>

namespace external_sort {


TempFileManager::TempFileManager(const std::string& base_dir_name) {
    base_temp_dir_path_ = std::filesystem::current_path() / base_dir_name;
    if (!std::filesystem::exists(base_temp_dir_path_)) {
        std::error_code ec;
        std::filesystem::create_directories(base_temp_dir_path_, ec);
        if (ec) {
            throw std::runtime_error(
                "TempFileManager failed to create temp directory: " + base_temp_dir_path_.string() +
                " Error: " + ec.message());
        }
        owns_directory_ = true;
        DEBUG_COUT_SUCCESS(
            "TempFileManager created temporary directory: " << base_temp_dir_path_ << std::endl);
    } else {
        owns_directory_ = false;
        DEBUG_COUT_INFO(
            "TempFileManager using existing temporary directory: "
            << base_temp_dir_path_ << std::endl);
    }
}

TempFileManager::~TempFileManager() {
    DEBUG_COUT_INFO("TempFileManager destructor: Attempting to clean up temporary directory..." << std::endl);
    if (owns_directory_ && std::filesystem::exists(base_temp_dir_path_)) {
        std::error_code ec;
        std::filesystem::remove_all(base_temp_dir_path_, ec);
        if (ec) {
            DEBUG_COUT_WARNING(
                "Warning: TempFileManager failed to remove_all temp directory "
                << base_temp_dir_path_ << ": " << ec.message() << std::endl);
        } else {
            DEBUG_COUT_SUCCESS(
                "TempFileManager removed temp directory and all its contents: "
                << base_temp_dir_path_ << std::endl);
        }
    } else if (std::filesystem::exists(base_temp_dir_path_)) {
         DEBUG_COUT_INFO(
            "TempFileManager: Temporary directory "
            << base_temp_dir_path_ << " exists but was not created by this instance, not removing." << std::endl);
    }
}

std::string TempFileManager::GenerateTempFilename(
    const std::string& prefix, const std::string& extension) {
    std::filesystem::path file_path =
        base_temp_dir_path_ / (prefix + std::to_string(temp_file_counter_++) + extension);
    DEBUG_COUT_SUCCESS("TempFileManager generated temp filename: " << file_path << std::endl);
    return file_path.string();
}

void TempFileManager::CleanupFile(const std::string& filename_str) {
    std::filesystem::path file_path(filename_str);
    if (std::filesystem::exists(file_path)) {
        std::error_code ec;
        std::filesystem::remove(file_path, ec);
        if (ec) {
             DEBUG_COUT_WARNING("CleanupFile failed to remove " << filename_str << ": " << ec.message() << std::endl);
        }
    }
}


const std::filesystem::path& TempFileManager::GetBaseDirPath() const {
    return base_temp_dir_path_;
}


}  // namespace external_sort
