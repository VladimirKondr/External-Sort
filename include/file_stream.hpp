/**
 * @file file_stream.hpp
 * @brief Файловые реализации потоков ввода-вывода
 * @author External Sort Library
 * @version 1.0
 */

#pragma once

#include "debug_logger.hpp"
#include "element_buffer.hpp"
#include "interfaces.hpp"
#include "temp_file_manager.hpp"

#include <cstdio>

namespace external_sort {

/**
 * @brief Файловая реализация входного потока
 *
 * Реализует интерфейс IInputStream для чтения данных из файла.
 * Использует буферизацию для эффективного чтения.
 *
 * @tparam T Тип элементов в потоке
 */
template <typename T>
class FileInputStream : public IInputStream<T> {
   private:
    StorageId id_;                         ///< Имя файла
    FILE* file_ptr_ = nullptr;             ///< Указатель на файл
    ElementBuffer<T> buffer_;              ///< Буфер для чтения
    uint64_t total_elements_in_file_ = 0;  ///< Общее количество элементов в файле
    uint64_t total_elements_read_ = 0;     ///< Количество прочитанных элементов
    bool is_exhausted_ = false;            ///< Флаг исчерпания потока
    T current_value_{};                    ///< Текущий элемент
    bool has_valid_value_ = false;         ///< Флаг валидности текущего элемента

    /**
     * @brief Заполняет внутренний буфер данными из файла
     */
    void FillBufferInternal();

   public:
    /**
     * @brief Конструктор файлового входного потока
     * @param filename Имя файла для чтения
     * @param buffer_capacity_elements Емкость буфера в элементах
     * @throws std::runtime_error если файл не может быть открыт
     */
    FileInputStream(const StorageId& filename, uint64_t buffer_capacity_elements);

    /**
     * @brief Деструктор, закрывающий файл
     */
    ~FileInputStream() override;

    FileInputStream(const FileInputStream&) = delete;
    FileInputStream& operator=(const FileInputStream&) = delete;

    FileInputStream(FileInputStream&& other) noexcept;
    FileInputStream& operator=(FileInputStream&& other) noexcept;

    /**
     * @brief Перемещает поток к следующему элементу
     */
    void Advance() override;

    /**
     * @brief Возвращает текущий элемент потока
     * @return Константная ссылка на текущий элемент
     * @throws std::logic_error если поток исчерпан
     */
    const T& Value() const override;

    /**
     * @brief Проверяет, исчерпан ли поток
     * @return true, если больше нет элементов для чтения
     */
    bool IsExhausted() const override;

    /**
     * @brief Проверяет, было ли исходное хранилище пустым
     * @return true, если файл не содержал элементов
     */
    bool IsEmptyOriginalStorage() const override;
};

/**
 * @brief Файловая реализация выходного потока
 *
 * Реализует интерфейс IOutputStream для записи данных в файл.
 * Использует буферизацию для эффективной записи.
 *
 * @tparam T Тип элементов в потоке
 */
template <typename T>
class FileOutputStream : public IOutputStream<T> {
   private:
    StorageId id_;                         ///< Имя файла
    FILE* file_ptr_ = nullptr;             ///< Указатель на файл
    ElementBuffer<T> buffer_;              ///< Буфер для записи
    uint64_t total_elements_written_ = 0;  ///< Количество записанных элементов
    bool finalized_ = false;               ///< Флаг финализации потока

    /**
     * @brief Сбрасывает буфер в файл
     */
    void FlushBufferInternal();

   public:
    /**
     * @brief Конструктор файлового выходного потока
     * @param filename Имя файла для записи
     * @param buffer_capacity_elements Емкость буфера в элементах
     * @throws std::runtime_error если файл не может быть создан
     */
    FileOutputStream(const StorageId& filename, uint64_t buffer_capacity_elements);

    /**
     * @brief Деструктор, финализирующий поток
     */
    ~FileOutputStream() override;

    FileOutputStream(const FileOutputStream&) = delete;
    FileOutputStream& operator=(const FileOutputStream&) = delete;

    FileOutputStream(FileOutputStream&& other) noexcept;
    FileOutputStream& operator=(FileOutputStream&& other) noexcept;

    /**
     * @brief Записывает элемент в поток
     * @param value Элемент для записи
     * @throws std::logic_error если поток финализирован
     */
    void Write(const T& value) override;

    /**
     * @brief Финализирует поток, записывая все буферизованные данные
     */
    void Finalize() override;

    /**
     * @brief Возвращает общее количество записанных элементов
     * @return Количество элементов, записанных в поток
     */
    uint64_t GetTotalElementsWritten() const override;

    /**
     * @brief Возвращает идентификатор файла
     * @return StorageId файла
     */
    StorageId GetId() const override;
};

/**
 * @brief Фабрика для файловых потоков
 *
 * Реализует интерфейс IStreamFactory для создания файловых потоков
 * ввода-вывода и управления временными файлами.
 *
 * @tparam T Тип элементов в потоках
 */
template <typename T>
class FileStreamFactory : public IStreamFactory<T> {
   private:
    TempFileManager temp_file_manager_;  ///< Менеджер временных файлов

   public:
    /**
     * @brief Конструктор фабрики файловых потоков
     * @param base_temp_dir_name Название базовой директории для временных файлов
     */
    explicit FileStreamFactory(const std::string& base_temp_dir_name = "temp_sort_runs_files");

    /**
     * @brief Создает входной поток для чтения из файла
     * @param id Путь к файлу
     * @param buffer_capacity_elements Емкость буфера
     * @return Уникальный указатель на FileInputStream
     */
    std::unique_ptr<IInputStream<T>> CreateInputStream(
        const StorageId& id, uint64_t buffer_capacity_elements) override;

    /**
     * @brief Создает выходной поток для записи в файл
     * @param id Путь к файлу
     * @param buffer_capacity_elements Емкость буфера
     * @return Уникальный указатель на FileOutputStream
     */
    std::unique_ptr<IOutputStream<T>> CreateOutputStream(
        const StorageId& id, uint64_t buffer_capacity_elements) override;

    /**
     * @brief Создает выходной поток для записи во временный файл
     * @param out_temp_id Выходной параметр для ID временного файла
     * @param buffer_capacity_elements Емкость буфера
     * @return Уникальный указатель на FileOutputStream
     */
    std::unique_ptr<IOutputStream<T>> CreateTempOutputStream(
        StorageId& out_temp_id, uint64_t buffer_capacity_elements) override;

    /**
     * @brief Удаляет файл
     * @param id Путь к файлу для удаления
     */
    void DeleteStorage(const StorageId& id) override;

    /**
     * @brief Делает временный файл постоянным
     * @param temp_id Путь к временному файлу
     * @param final_id Путь к конечному файлу
     */
    void MakeStoragePermanent(const StorageId& temp_id, const StorageId& final_id) override;

    /**
     * @brief Проверяет существование файла
     * @param id Путь к файлу
     * @return true, если файл существует
     */
    bool StorageExists(const StorageId& id) const override;

    /**
     * @brief Возвращает контекст временных файлов
     * @return Путь к директории временных файлов
     */
    StorageId GetTempStorageContextId() const override;
};

#include "impl/file_stream.tpp"

}  // namespace external_sort
