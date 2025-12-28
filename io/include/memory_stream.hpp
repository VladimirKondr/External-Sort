/**
 * @file memory_stream.hpp
 * @brief In-memory implementations of input/output streams
 */

#pragma once

#include "io_logging.hpp"
#include "interfaces.hpp"

#include <map>
#include <memory>
#include <vector>

namespace io {

/**
 * @brief In-memory implementation of output stream
 *
 * Implements the IOutputStream interface for writing data to memory.
 * All data is stored in std::vector.
 *
 * @tparam T Type of elements in the stream
 */
template <typename T>
class InMemoryOutputStream : public IOutputStream<T> {
   private:
    StorageId id_;                               ///< Stream identifier
    std::shared_ptr<std::vector<T>> data_ptr_;   ///< Pointer to data
    std::shared_ptr<uint64_t> actual_size_ptr_;  ///< Pointer to size ("header")
    uint64_t elements_written_ = 0;              ///< Number of elements written
    bool finalized_ = false;                     ///< Finalization flag

   public:
    /**
     * @brief Constructor for in-memory output stream
     * @param id Stream identifier
     * @param data_vec_ptr Pointer to data vector
     * @param size_ptr Pointer to data size
     * @param buffer_capacity Buffer capacity (not used in in-memory)
     */
    InMemoryOutputStream(
        StorageId id, std::shared_ptr<std::vector<T>> data_vec_ptr,
        std::shared_ptr<uint64_t> size_ptr, [[maybe_unused]] uint64_t buffer_capacity);

    /**
     * @brief Destructor that finalizes the stream
     */
    ~InMemoryOutputStream() override;

    /**
     * @brief Writes an element to the stream
     * @param value Element to write
     * @throws std::logic_error if the stream is finalized
     */
    void Write(const T& value) override;

    /**
     * @brief Finalizes the stream
     */
    void Finalize() override;

    /**
     * @brief Returns the total number of elements written
     * @return Number of elements written to the stream
     */
    uint64_t GetTotalElementsWritten() const override;

    /**
     * @brief Returns the stream identifier
     * @return Stream StorageId
     */
    StorageId GetId() const override;
};

/**
 * @brief In-memory implementation of input stream
 *
 * Implements the IInputStream interface for reading data from memory.
 * Reads data from std::vector.
 *
 * @tparam T Type of elements in the stream
 */
template <typename T>
class InMemoryInputStream : public IInputStream<T> {
   private:
    StorageId id_;                                    ///< Stream identifier
    std::shared_ptr<const std::vector<T>> data_ptr_;  ///< Pointer to data (read-only)
    uint64_t total_elements_in_storage_;              ///< Total number of elements in storage
    uint64_t read_cursor_ = 0;                        ///< Read cursor
    T current_value_{};                               ///< Current element
    bool has_valid_value_ = false;                    ///< Current element validity flag
    bool is_exhausted_ = false;                       ///< Stream exhaustion flag

   public:
    /**
     * @brief Constructor for in-memory input stream
     * @param id Stream identifier
     * @param data_vec_ptr Pointer to data vector
     * @param actual_storage_size Actual data size
     * @param buffer_capacity Buffer capacity (not used in in-memory)
     */
    InMemoryInputStream(
        StorageId id, std::shared_ptr<const std::vector<T>> data_vec_ptr,
        uint64_t actual_storage_size, [[maybe_unused]] uint64_t buffer_capacity);

    /**
     * @brief Destructor
     */
    ~InMemoryInputStream() override = default;

    /**
     * @brief Advances the stream to the next element
     */
    void Advance() override;

    /**
     * @brief Returns the current element of the stream
     * @return Constant reference to the current element
     * @throws std::logic_error if the stream is exhausted
     */
    const T& Value() const override;

    /**
     * @brief Checks if the stream is exhausted
     * @return true if there are no more elements to read
     */
    bool IsExhausted() const override;

    /**
     * @brief Checks if the original storage was empty
     * @return true if the storage contained no elements
     */
    bool IsEmptyOriginalStorage() const override;
};

/**
 * @brief Factory for in-memory streams
 *
 * Implements the IStreamFactory interface for creating streams
 * that work with data in memory.
 *
 * @tparam T Type of elements in the streams
 */
template <typename T>
class InMemoryStreamFactory : public IStreamFactory<T> {
   private:
    std::map<StorageId, std::shared_ptr<std::vector<T>>> storages_;  ///< Data storages
    std::map<StorageId, std::shared_ptr<uint64_t>>
        storage_declared_sizes_;                             ///< Declared storage sizes
    uint64_t temp_id_counter_ = 0;                           ///< Temporary ID counter
    const std::string temp_prefix_ = "in_memory_temp_run_";  ///< Temporary ID prefix

   public:
    /**
     * @brief Constructor for in-memory stream factory
     */
    InMemoryStreamFactory() = default;

    /**
     * @brief Creates an input stream for reading from memory
     * @param id Storage identifier
     * @param buffer_capacity_elements Buffer capacity (not used)
     * @return Unique pointer to InMemoryInputStream
     * @throws std::runtime_error if storage is not found
     */
    std::unique_ptr<IInputStream<T>> CreateInputStream(
        const StorageId& id, uint64_t buffer_capacity_elements) override;

    /**
     * @brief Creates an output stream for writing to memory
     * @param id Storage identifier
     * @param buffer_capacity_elements Buffer capacity (not used)
     * @return Unique pointer to InMemoryOutputStream
     */
    std::unique_ptr<IOutputStream<T>> CreateOutputStream(
        const StorageId& id, uint64_t buffer_capacity_elements) override;

    /**
     * @brief Creates an output stream for writing to temporary storage
     * @param out_temp_id Output parameter for temporary storage ID
     * @param buffer_capacity_elements Buffer capacity (not used)
     * @return Unique pointer to InMemoryOutputStream
     */
    std::unique_ptr<IOutputStream<T>> CreateTempOutputStream(
        StorageId& out_temp_id, uint64_t buffer_capacity_elements) override;

    /**
     * @brief Deletes a storage from memory
     * @param id Storage identifier to delete
     */
    void DeleteStorage(const StorageId& id) override;

    /**
     * @brief Makes a temporary storage permanent
     * @param temp_id Temporary storage identifier
     * @param final_id Final storage identifier
     * @throws std::runtime_error if temporary storage is not found
     */
    void MakeStoragePermanent(const StorageId& temp_id, const StorageId& final_id) override;

    /**
     * @brief Checks if a storage exists
     * @param id Storage identifier
     * @return true if the storage exists
     */
    bool StorageExists(const StorageId& id) const override;

    /**
     * @brief Returns the temporary storage context
     * @return Temporary ID prefix
     */
    StorageId GetTempStorageContextId() const override;

    /**
     * @brief Returns storage data (for tests)
     * @param id Storage identifier
     * @return Pointer to data or nullptr if not found
     */
    std::shared_ptr<const std::vector<T>> GetStorageData(const StorageId& id) const;

    /**
     * @brief Returns declared storage size (for tests)
     * @param id Storage identifier
     * @return Declared size or 0 if not found
     */
    uint64_t GetStorageDeclaredSize(const StorageId& id) const;
};

template <typename T>
InMemoryOutputStream<T>::InMemoryOutputStream(
    StorageId id, std::shared_ptr<std::vector<T>> data_vec_ptr, std::shared_ptr<uint64_t> size_ptr,
    [[maybe_unused]] uint64_t buffer_capacity)
    : id_(std::move(id))
    , data_ptr_(std::move(data_vec_ptr))
    , actual_size_ptr_(std::move(size_ptr)) {
    data_ptr_->clear();
    *actual_size_ptr_ = 0;
}

template <typename T>
InMemoryOutputStream<T>::~InMemoryOutputStream() {
    Finalize();
}

template <typename T>
void InMemoryOutputStream<T>::Write(const T& value) {
    if (finalized_) {
        throw std::logic_error("Write to finalized InMemoryOutputStream: " + id_);
    }
    data_ptr_->push_back(value);
    elements_written_++;
}

template <typename T>
void InMemoryOutputStream<T>::Finalize() {
    if (finalized_) {
        return;
    }
    *actual_size_ptr_ = elements_written_;
    finalized_ = true;
    detail::LogInfo("InMemoryOutputStream: Finalized " + id_ + 
                    ". Elements: " + std::to_string(*actual_size_ptr_));
}

template <typename T>
uint64_t InMemoryOutputStream<T>::GetTotalElementsWritten() const {
    return elements_written_;
}

template <typename T>
StorageId InMemoryOutputStream<T>::GetId() const {
    return id_;
}

template <typename T>
InMemoryInputStream<T>::InMemoryInputStream(
    StorageId id, std::shared_ptr<const std::vector<T>> data_vec_ptr, uint64_t actual_storage_size,
    [[maybe_unused]] uint64_t buffer_capacity)
    : id_(std::move(id))
    , data_ptr_(std::move(data_vec_ptr))
    , total_elements_in_storage_(actual_storage_size) {
    if (total_elements_in_storage_ > data_ptr_->size()) {
        detail::LogWarning("Warning: InMemoryInputStream " + id_ + 
                          " declared size (" + std::to_string(total_elements_in_storage_) + 
                          ") > actual vector size (" + std::to_string(data_ptr_->size()) + 
                          "). Clamping to actual size.");
        total_elements_in_storage_ = data_ptr_->size();
    }

    if (total_elements_in_storage_ == 0) {
        is_exhausted_ = true;
        has_valid_value_ = false;
    } else {
        Advance();
    }
}

template <typename T>
void InMemoryInputStream<T>::Advance() {
    if (is_exhausted_ || read_cursor_ >= total_elements_in_storage_) {
        has_valid_value_ = false;
        is_exhausted_ = true;
        return;
    }
    current_value_ = (*data_ptr_)[read_cursor_];
    read_cursor_++;
    has_valid_value_ = true;
    if (read_cursor_ >= total_elements_in_storage_) {
        is_exhausted_ = true;
    }
}

template <typename T>
const T& InMemoryInputStream<T>::Value() const {
    if (!has_valid_value_) {
        throw std::logic_error("Value from exhausted InMemoryInputStream: " + id_);
    }
    return current_value_;
}

template <typename T>
bool InMemoryInputStream<T>::IsExhausted() const {
    return is_exhausted_ && !has_valid_value_;
}

template <typename T>
bool InMemoryInputStream<T>::IsEmptyOriginalStorage() const {
    return total_elements_in_storage_ == 0;
}

template <typename T>
std::unique_ptr<IInputStream<T>> InMemoryStreamFactory<T>::CreateInputStream(
    const StorageId& id, uint64_t buffer_capacity_elements) {
    auto it_data = storages_.find(id);
    auto it_size = storage_declared_sizes_.find(id);
    if (it_data == storages_.end() || it_size == storage_declared_sizes_.end()) {
        throw std::runtime_error("InMemoryStreamFactory: Storage ID not found for input: " + id);
    }
    return std::make_unique<InMemoryInputStream<T>>(
        id, it_data->second, *(it_size->second), buffer_capacity_elements);
}

template <typename T>
std::unique_ptr<IOutputStream<T>> InMemoryStreamFactory<T>::CreateOutputStream(
    const StorageId& id, uint64_t buffer_capacity_elements) {
    auto data_ptr = std::make_shared<std::vector<T>>();
    auto size_ptr = std::make_shared<uint64_t>(0);
    storages_[id] = data_ptr;
    storage_declared_sizes_[id] = size_ptr;
    return std::make_unique<InMemoryOutputStream<T>>(
        id, data_ptr, size_ptr, buffer_capacity_elements);
}

template <typename T>
std::unique_ptr<IOutputStream<T>> InMemoryStreamFactory<T>::CreateTempOutputStream(
    StorageId& out_temp_id, uint64_t buffer_capacity_elements) {
    out_temp_id = temp_prefix_ + std::to_string(temp_id_counter_++);
    auto data_ptr = std::make_shared<std::vector<T>>();
    auto size_ptr = std::make_shared<uint64_t>(0);
    storages_[out_temp_id] = data_ptr;
    storage_declared_sizes_[out_temp_id] = size_ptr;
    return std::make_unique<InMemoryOutputStream<T>>(
        out_temp_id, data_ptr, size_ptr, buffer_capacity_elements);
}

template <typename T>
void InMemoryStreamFactory<T>::DeleteStorage(const StorageId& id) {
    storages_.erase(id);
    storage_declared_sizes_.erase(id);
    detail::LogInfo("InMemoryStreamFactory: Deleted storage " + id);
}

template <typename T>
void InMemoryStreamFactory<T>::MakeStoragePermanent(
    const StorageId& temp_id, const StorageId& final_id) {
    if (temp_id == final_id) {
        return;
    }

    auto it_temp_data = storages_.find(temp_id);
    auto it_temp_size = storage_declared_sizes_.find(temp_id);
    if (it_temp_data == storages_.end() || it_temp_size == storage_declared_sizes_.end()) {
        throw std::runtime_error("InMemoryStreamFactory: Temp ID not found: " + temp_id);
    }
    storages_[final_id] = it_temp_data->second;
    storage_declared_sizes_[final_id] = it_temp_size->second;

    storages_.erase(it_temp_data);
    storage_declared_sizes_.erase(it_temp_size);
    detail::LogInfo("InMemoryStreamFactory: Made " + temp_id + " permanent as " + final_id);
}

template <typename T>
bool InMemoryStreamFactory<T>::StorageExists(const StorageId& id) const {
    return storages_.count(id);
}

template <typename T>
StorageId InMemoryStreamFactory<T>::GetTempStorageContextId() const {
    return temp_prefix_;
}

template <typename T>
std::shared_ptr<const std::vector<T>> InMemoryStreamFactory<T>::GetStorageData(
    const StorageId& id) const {
    auto it = storages_.find(id);
    return (it != storages_.end()) ? it->second : nullptr;
}

template <typename T>
uint64_t InMemoryStreamFactory<T>::GetStorageDeclaredSize(const StorageId& id) const {
    auto it = storage_declared_sizes_.find(id);
    return (it != storage_declared_sizes_.end()) ? *(it->second) : 0;
}


}  // namespace io
