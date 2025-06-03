/**
 * @file interfaces.hpp
 * @brief Интерфейсы для входных и выходных потоков, а также фабрики потоков
 * @author External Sort Library
 * @version 1.0
 */

#pragma once

#include "storage_types.hpp"

#include <cstdint>
#include <memory>

namespace external_sort {

template <typename T>
class IInputStream;

template <typename T>
class IOutputStream;

/**
 * @brief Интерфейс для фабрики потоков
 *
 * Управляет созданием, удалением и финализацией потоков данных,
 * которые могут быть файловыми или внутри-памятными.
 *
 * @tparam T Тип элементов в потоках
 */
template <typename T>
class IStreamFactory {
   public:
    /**
     * @brief Виртуальный деструктор
     */
    virtual ~IStreamFactory() = default;

    /**
     * @brief Создает входной поток для чтения из существующего хранилища
     * @param id Идентификатор хранилища для чтения
     * @param buffer_capacity_elements Емкость внутреннего буфера для потока
     * @return Уникальный указатель на IInputStream
     * @throws std::runtime_error если хранилище не может быть открыто или не существует
     */
    virtual std::unique_ptr<IInputStream<T>> CreateInputStream(
        const StorageId& id, uint64_t buffer_capacity_elements) = 0;

    /**
     * @brief Создает выходной поток для записи в новое или существующее хранилище
     *
     * Если хранилище существует, его содержимое может быть перезаписано.
     *
     * @param id Идентификатор хранилища для записи
     * @param buffer_capacity_elements Емкость внутреннего буфера для потока
     * @return Уникальный указатель на IOutputStream
     * @throws std::runtime_error если хранилище не может быть создано или открыто для записи
     */
    virtual std::unique_ptr<IOutputStream<T>> CreateOutputStream(
        const StorageId& id, uint64_t buffer_capacity_elements) = 0;

    /**
     * @brief Создает выходной поток для записи в новое временное хранилище
     *
     * Фабрика назначает уникальный ID этому временному хранилищу.
     *
     * @param out_temp_id Выходной параметр, который получит ID созданного временного хранилища
     * @param buffer_capacity_elements Емкость внутреннего буфера для потока
     * @return Уникальный указатель на IOutputStream для временного хранилища
     */
    virtual std::unique_ptr<IOutputStream<T>> CreateTempOutputStream(
        StorageId& out_temp_id, uint64_t buffer_capacity_elements) = 0;

    /**
     * @brief Удаляет хранилище
     * @param id Идентификатор хранилища для удаления
     */
    virtual void DeleteStorage(const StorageId& id) = 0;

    /**
     * @brief Делает временное хранилище постоянным под новым (или тем же) ID
     *
     * Обычно включает переименование или копирование. Исходное временное хранилище (temp_id)
     * обычно удаляется или становится недоступным после этой операции.
     *
     * @param temp_id Идентификатор временного исходного хранилища
     * @param final_id Идентификатор для конечного постоянного хранилища
     * @throws std::runtime_error если операция не удается
     */
    virtual void MakeStoragePermanent(const StorageId& temp_id, const StorageId& final_id) = 0;

    /**
     * @brief Проверяет, существует ли хранилище с данным ID
     * @param id Идентификатор хранилища
     * @return true, если хранилище существует, иначе false
     */
    virtual bool StorageExists(const StorageId& id) const = 0;

    /**
     * @brief Получает идентификатор, представляющий базовый путь или контекст для временных
     * хранилищ
     *
     * Используется для предотвращения конфликтов (например, вывод в саму временную директорию).
     *
     * @return StorageId, представляющий контекст временных хранилищ
     */
    virtual StorageId GetTempStorageContextId() const = 0;
};

/**
 * @brief Интерфейс для входного потока элементов типа T
 * @tparam T Тип элементов в потоке
 */
template <typename T>
class IInputStream {
   public:
    /**
     * @brief Виртуальный деструктор
     */
    virtual ~IInputStream() = default;

    /**
     * @brief Перемещает поток к следующему элементу
     */
    virtual void Advance() = 0;

    /**
     * @brief Возвращает текущий элемент потока
     * @return Константная ссылка на текущий элемент
     * @throws std::logic_error если поток исчерпан
     */
    virtual const T& Value() const = 0;

    /**
     * @brief Проверяет, исчерпан ли поток
     * @return true, если больше нет элементов для чтения
     */
    virtual bool IsExhausted() const = 0;

    /**
     * @brief Проверяет, было ли исходное хранилище пустым
     * @return true, если исходное хранилище не содержало элементов
     */
    virtual bool IsEmptyOriginalStorage() const = 0;
};

/**
 * @brief Интерфейс для выходного потока элементов типа T
 * @tparam T Тип элементов в потоке
 */
template <typename T>
class IOutputStream {
   public:
    /**
     * @brief Виртуальный деструктор
     */
    virtual ~IOutputStream() = default;

    /**
     * @brief Записывает элемент в поток
     * @param value Элемент для записи
     * @throws std::logic_error если поток финализирован
     */
    virtual void Write(const T& value) = 0;

    /**
     * @brief Финализирует поток, записывая все буферизованные данные
     */
    virtual void Finalize() = 0;

    /**
     * @brief Возвращает общее количество записанных элементов
     * @return Количество элементов, записанных в поток
     */
    virtual uint64_t GetTotalElementsWritten() const = 0;

    /**
     * @brief Возвращает идентификатор хранилища
     * @return StorageId хранилища, связанного с потоком
     */
    virtual StorageId GetId() const = 0;
};

}  // namespace external_sort
