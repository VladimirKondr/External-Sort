/**
 * @file interfaces.hpp
 * @brief Base interfaces for input and output strems and for stream factories
 */

#pragma once

#include "storage_types.hpp"

#include <cstdint>
#include <memory>

namespace io {

/**
 * @brief Interface for input stream of elements of type T
 * @tparam T type of elements in stream
 */
template <typename T>
class IInputStream {
   public:
    virtual ~IInputStream() = default;

    /**
     * @brief Advances stream to the next element
     */
    virtual void Advance() = 0;

    /**
     * @brief Returns current element of the stream
     * @return Constant reference to the current element of the stream
     * @throws std::logic_error if the stream is exhausted
     */
    virtual const T& Value() const = 0;

    /**
     * @brief Checks if the stream is exhausted
     * @return true, if there are no more elements to read
     */
    virtual bool IsExhausted() const = 0;

    /**
     * @brief Checks if original storage is empty
     * @return true, if original storage doesn't contains eny elements
     */
    virtual bool IsEmptyOriginalStorage() const = 0;
};

/**
 * @brief Interface for output stream of elements of type T
 * @tparam T type of elements in stream
 */
template <typename T>
class IOutputStream {
   public:
    virtual ~IOutputStream() = default;

    /**
     * @brief Writes an element to the stream
     * @param value The element to write
     * @throws std::logic_error if the stream has been finalized
     */
    virtual void Write(const T& value) = 0;

    /**
     * @brief Finalizes the stream, writing all buffered data
     */
    virtual void Finalize() = 0;

    /**
     * @brief Returns the total number of elements written
     * @return The number of elements written to the stream
     */
    virtual uint64_t GetTotalElementsWritten() const = 0;

    /**
     * @brief Returns the identifier of the storage
     * @return The StorageId of the storage associated with the stream
     */
    virtual StorageId GetId() const = 0;
};

/**
 * @brief Interface for a stream factory
 *
 * Manages creation, deletion, and finalization of data streams,
 * which can be either file-based or in-memory.
 *
 * @tparam T The type of elements in the streams
 */
template <typename T>
class IStreamFactory {
   public:
    virtual ~IStreamFactory() = default;

    /**
     * @brief Creates an input stream to read from an existing storage
     * @param id The identifier of the storage to read from
     * @param buffer_capacity_elements The capacity of the stream's internal buffer
     * @return A unique pointer to an IInputStream
     * @throws std::runtime_error if the storage cannot be opened or does not exist
     */
    virtual std::unique_ptr<IInputStream<T>> CreateInputStream(
        const StorageId& id, uint64_t buffer_capacity_elements) = 0;

    /**
     * @brief Creates an output stream to write to a new or existing storage
     *
     * If the storage already exists, its content will be overwritten.
     *
     * @param id The identifier of the storage to write to
     * @param buffer_capacity_elements The capacity of the stream's internal buffer
     * @return A unique pointer to an IOutputStream
     * @throws std::runtime_error if the storage cannot be created or opened for writing
     */
    virtual std::unique_ptr<IOutputStream<T>> CreateOutputStream(
        const StorageId& id, uint64_t buffer_capacity_elements) = 0;

    /**
     * @brief Creates an output stream to write to a new temporary storage
     *
     * The factory assigns a unique ID to this temporary storage.
     *
     * @param out_temp_id Output parameter that will receive the ID of the created temporary storage
     * @param buffer_capacity_elements The capacity of the stream's internal buffer
     * @return A unique pointer to an IOutputStream for the temporary storage
     */
    virtual std::unique_ptr<IOutputStream<T>> CreateTempOutputStream(
        StorageId& out_temp_id, uint64_t buffer_capacity_elements) = 0;

    /**
     * @brief Deletes a storage
     * @param id The identifier of the storage to delete
     */
    virtual void DeleteStorage(const StorageId& id) = 0;

    /**
     * @brief Makes a temporary storage permanent under a new (or the same) ID
     *
     * This typically involves renaming or copying. The original temporary storage (temp_id)
     * is usually deleted or becomes inaccessible after this operation.
     *
     * @param temp_id The identifier of the source temporary storage
     * @param final_id The identifier for the destination permanent storage
     * @throws std::runtime_error if the operation fails
     */
    virtual void MakeStoragePermanent(const StorageId& temp_id, const StorageId& final_id) = 0;

    /**
     * @brief Checks if a storage with the given ID exists
     * @param id The identifier of the storage to check
     * @return true if the storage exists, false otherwise
     */
    virtual bool StorageExists(const StorageId& id) const = 0;

    /**
     * @brief Gets an identifier representing the base path or context for temporary storages
     * This is used to prevent conflicts (e.g., writing output to the temporary directory itself).
     * @return A StorageId representing the context for temporary storages
     */
    virtual StorageId GetTempStorageContextId() const = 0;
};

}  // namespace io