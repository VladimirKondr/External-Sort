/**
 * @file memory_stream.tpp
 * @brief Реализация in-memory потоков
 * @author External Sort Library
 * @version 1.0
 */

#pragma once

#include <iostream>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

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
    DEBUG_COUT_SUCCESS(
        "InMemoryOutputStream: Finalized "
        << id_ << ". Elements: " << *actual_size_ptr_ << std::endl);
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
        DEBUG_COUT_WARNING(
            "Warning: InMemoryInputStream "
            << id_ << " declared size (" << total_elements_in_storage_ << ") > actual vector size ("
            << data_ptr_->size() << "). Clamping to actual size." << std::endl);
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
    DEBUG_COUT_INFO("InMemoryStreamFactory: Deleted storage " << id << std::endl);
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
    DEBUG_COUT_SUCCESS(
        "InMemoryStreamFactory: Made " << temp_id << " permanent as " << final_id << std::endl);
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
