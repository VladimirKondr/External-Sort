/**
 * @file memory_stream.hpp
 * @brief In-memory реализации потоков ввода-вывода
 * @author External Sort Library
 * @version 1.0
 */

#pragma once

#include "debug_logger.hpp"
#include "interfaces.hpp"

#include <map>
#include <memory>
#include <vector>

namespace external_sort {

/**
 * @brief In-memory реализация выходного потока
 *
 * Реализует интерфейс IOutputStream для записи данных в память.
 * Все данные хранятся в std::vector.
 *
 * @tparam T Тип элементов в потоке
 */
template <typename T>
class InMemoryOutputStream : public IOutputStream<T> {
   private:
    StorageId id_;                               ///< Идентификатор потока
    std::shared_ptr<std::vector<T>> data_ptr_;   ///< Указатель на данные
    std::shared_ptr<uint64_t> actual_size_ptr_;  ///< Указатель на размер ("заголовок")
    uint64_t elements_written_ = 0;              ///< Количество записанных элементов
    bool finalized_ = false;                     ///< Флаг финализации

   public:
    /**
     * @brief Конструктор in-memory выходного потока
     * @param id Идентификатор потока
     * @param data_vec_ptr Указатель на вектор данных
     * @param size_ptr Указатель на размер данных
     * @param buffer_capacity Емкость буфера (не используется в in-memory)
     */
    InMemoryOutputStream(
        StorageId id, std::shared_ptr<std::vector<T>> data_vec_ptr,
        std::shared_ptr<uint64_t> size_ptr, [[maybe_unused]] uint64_t buffer_capacity);

    /**
     * @brief Деструктор, финализирующий поток
     */
    ~InMemoryOutputStream() override;

    /**
     * @brief Записывает элемент в поток
     * @param value Элемент для записи
     * @throws std::logic_error если поток финализирован
     */
    void Write(const T& value) override;

    /**
     * @brief Финализирует поток
     */
    void Finalize() override;

    /**
     * @brief Возвращает общее количество записанных элементов
     * @return Количество элементов, записанных в поток
     */
    uint64_t GetTotalElementsWritten() const override;

    /**
     * @brief Возвращает идентификатор потока
     * @return StorageId потока
     */
    StorageId GetId() const override;
};

/**
 * @brief In-memory реализация входного потока
 *
 * Реализует интерфейс IInputStream для чтения данных из памяти.
 * Читает данные из std::vector.
 *
 * @tparam T Тип элементов в потоке
 */
template <typename T>
class InMemoryInputStream : public IInputStream<T> {
   private:
    StorageId id_;                                    ///< Идентификатор потока
    std::shared_ptr<const std::vector<T>> data_ptr_;  ///< Указатель на данные (только чтение)
    uint64_t total_elements_in_storage_;              ///< Общее количество элементов в хранилище
    uint64_t read_cursor_ = 0;                        ///< Курсор чтения
    T current_value_{};                               ///< Текущий элемент
    bool has_valid_value_ = false;                    ///< Флаг валидности текущего элемента
    bool is_exhausted_ = false;                       ///< Флаг исчерпания потока

   public:
    /**
     * @brief Конструктор in-memory входного потока
     * @param id Идентификатор потока
     * @param data_vec_ptr Указатель на вектор данных
     * @param actual_storage_size Реальный размер данных
     * @param buffer_capacity Емкость буфера (не используется в in-memory)
     */
    InMemoryInputStream(
        StorageId id, std::shared_ptr<const std::vector<T>> data_vec_ptr,
        uint64_t actual_storage_size, [[maybe_unused]] uint64_t buffer_capacity);

    /**
     * @brief Деструктор
     */
    ~InMemoryInputStream() override = default;

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
     * @return true, если хранилище не содержало элементов
     */
    bool IsEmptyOriginalStorage() const override;
};

/**
 * @brief Фабрика для in-memory потоков
 *
 * Реализует интерфейс IStreamFactory для создания потоков,
 * работающих с данными в памяти.
 *
 * @tparam T Тип элементов в потоках
 */
template <typename T>
class InMemoryStreamFactory : public IStreamFactory<T> {
   private:
    std::map<StorageId, std::shared_ptr<std::vector<T>>> storages_;  ///< Хранилища данных
    std::map<StorageId, std::shared_ptr<uint64_t>>
        storage_declared_sizes_;                             ///< Заявленные размеры хранилищ
    uint64_t temp_id_counter_ = 0;                           ///< Счетчик временных ID
    const std::string temp_prefix_ = "in_memory_temp_run_";  ///< Префикс временных ID

   public:
    /**
     * @brief Конструктор фабрики in-memory потоков
     */
    InMemoryStreamFactory() = default;

    /**
     * @brief Создает входной поток для чтения из памяти
     * @param id Идентификатор хранилища
     * @param buffer_capacity_elements Емкость буфера (не используется)
     * @return Уникальный указатель на InMemoryInputStream
     * @throws std::runtime_error если хранилище не найдено
     */
    std::unique_ptr<IInputStream<T>> CreateInputStream(
        const StorageId& id, uint64_t buffer_capacity_elements) override;

    /**
     * @brief Создает выходной поток для записи в память
     * @param id Идентификатор хранилища
     * @param buffer_capacity_elements Емкость буфера (не используется)
     * @return Уникальный указатель на InMemoryOutputStream
     */
    std::unique_ptr<IOutputStream<T>> CreateOutputStream(
        const StorageId& id, uint64_t buffer_capacity_elements) override;

    /**
     * @brief Создает выходной поток для записи во временное хранилище
     * @param out_temp_id Выходной параметр для ID временного хранилища
     * @param buffer_capacity_elements Емкость буфера (не используется)
     * @return Уникальный указатель на InMemoryOutputStream
     */
    std::unique_ptr<IOutputStream<T>> CreateTempOutputStream(
        StorageId& out_temp_id, uint64_t buffer_capacity_elements) override;

    /**
     * @brief Удаляет хранилище из памяти
     * @param id Идентификатор хранилища для удаления
     */
    void DeleteStorage(const StorageId& id) override;

    /**
     * @brief Делает временное хранилище постоянным
     * @param temp_id Идентификатор временного хранилища
     * @param final_id Идентификатор конечного хранилища
     * @throws std::runtime_error если временное хранилище не найдено
     */
    void MakeStoragePermanent(const StorageId& temp_id, const StorageId& final_id) override;

    /**
     * @brief Проверяет существование хранилища
     * @param id Идентификатор хранилища
     * @return true, если хранилище существует
     */
    bool StorageExists(const StorageId& id) const override;

    /**
     * @brief Возвращает контекст временных хранилищ
     * @return Префикс временных ID
     */
    StorageId GetTempStorageContextId() const override;

    /**
     * @brief Возвращает данные хранилища (для тестов)
     * @param id Идентификатор хранилища
     * @return Указатель на данные или nullptr, если не найдено
     */
    std::shared_ptr<const std::vector<T>> GetStorageData(const StorageId& id) const;

    /**
     * @brief Возвращает заявленный размер хранилища (для тестов)
     * @param id Идентификатор хранилища
     * @return Заявленный размер или 0, если не найдено
     */
    uint64_t GetStorageDeclaredSize(const StorageId& id) const;
};

#include "impl/memory_stream.tpp"

}  // namespace external_sort
