/**
 * @file temp_file_manager.hpp
 * @brief Менеджер временных файлов для внешней сортировки
 * @author External Sort Library
 * @version 1.0
 */

#pragma once

#include <cstdint>
#include <filesystem>
#include <set>
#include <string>

namespace external_sort {

/**
 * @brief Менеджер для управления временными файлами
 *
 * Класс отвечает за создание, отслеживание и удаление временных файлов,
 * используемых в процессе внешней сортировки.
 */
class TempFileManager {
   private:
    std::filesystem::path base_temp_dir_path_;  ///< Базовая директория для временных файлов
    uint64_t temp_file_counter_ = 0;            ///< Счетчик для генерации уникальных имен
    bool owns_directory_{};                     ///< Флаг владения директорией

   public:
    /**
     * @brief Конструктор менеджера временных файлов
     * @param base_dir_name Название базовой директории для временных файлов
     */
    explicit TempFileManager(const std::string& base_dir_name = "ts");

    /**
     * @brief Деструктор, очищающий все временные файлы
     */
    ~TempFileManager();

    TempFileManager(const TempFileManager&) = delete;
    TempFileManager& operator=(const TempFileManager&) = delete;

    TempFileManager(TempFileManager&&) = default;
    TempFileManager& operator=(TempFileManager&&) = default;

    /**
     * @brief Генерирует уникальное имя временного файла
     * @param prefix Префикс имени файла
     * @param extension Расширение файла
     * @return Полный путь к временному файлу
     */
    std::string GenerateTempFilename(
        const std::string& prefix = "r", const std::string& extension = ".b");

    /**
     * @brief Удаляет файл и прекращает его отслеживание
     * @param filename_str Путь к файлу для удаления
     */
    void CleanupFile(const std::string& filename_str);

    /**
     * @brief Возвращает путь к базовой директории
     * @return Константная ссылка на путь к базовой директории
     */
    const std::filesystem::path& GetBaseDirPath() const;
};

}  // namespace external_sort
