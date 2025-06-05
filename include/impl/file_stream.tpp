/**
 * @file file_stream.tpp
 * @brief Реализация файловых потоков
 * @author External Sort Library
 * @version 1.0
 */

#pragma once

#include "serializers.hpp"

#include <cstdio>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <utility>

template <typename T>
void FileInputStream<T>::FillBufferInternal() {
    if (is_exhausted_ ||
        (total_elements_in_file_ > 0 && total_elements_read_ >= total_elements_in_file_)) {
        buffer_.SetValidElementsCount(0);
        is_exhausted_ = true;
        has_valid_value_ = false;
        return;
    }
    uint64_t elements_to_read_this_pass =
        std::min(buffer_.Capacity(), total_elements_in_file_ - total_elements_read_);

    if (elements_to_read_this_pass == 0) {
        buffer_.SetValidElementsCount(0);
        is_exhausted_ = true;
        has_valid_value_ = false;
        return;
    }

    buffer_.Clear();

    uint64_t elements_actually_read = 0;
    if constexpr (PodSerializable<T>) {
        elements_actually_read =
            fread(buffer_.RawDataPtr(), sizeof(T), elements_to_read_this_pass, file_ptr_);
        buffer_.SetValidElementsCount(elements_actually_read);
    } else {
        for (uint64_t i = 0; i < elements_to_read_this_pass; ++i) {
            T element;
            if (!serializer_member_->Deserialize(element, file_ptr_)) {
                break;
            }
            buffer_.PushBack(element);
            elements_actually_read++;
        }
    }

    if (elements_actually_read < elements_to_read_this_pass && !feof(file_ptr_)) {
        if (ferror(file_ptr_)) {
            throw std::runtime_error(
                "FileInputStream: Error reading from file: " + id_ + " (ferror set)");
        }
        throw std::runtime_error(
            "FileInputStream: Failed to read expected elements from file: " + id_);
    }
    if (elements_actually_read == 0 && total_elements_read_ < total_elements_in_file_ &&
        feof(file_ptr_)) {
        is_exhausted_ = true;
        has_valid_value_ = false;
    }
}

template <typename T>
FileInputStream<T>::FileInputStream(const StorageId& filename, uint64_t buffer_capacity_elements)
    : id_(filename), buffer_(buffer_capacity_elements) {
    if constexpr (!PodSerializable<T>) {
        serializer_member_ = create_serializer<T>();
    }
    file_ptr_ = fopen(id_.c_str(), "rb");
    if (!file_ptr_) {
        throw std::runtime_error("FileInputStream: Cannot open input file: " + id_);
    }
    if (setvbuf(file_ptr_, nullptr, _IONBF, 0) != 0) {
        DEBUG_COUT_WARNING(
            "Warning: FileInputStream could not disable stdio buffering for " << id_ << std::endl);
    }

    if (fread(&total_elements_in_file_, sizeof(uint64_t), 1, file_ptr_) != 1) {
        if (feof(file_ptr_) && std::filesystem::file_size(id_) < sizeof(uint64_t)) {
            total_elements_in_file_ = 0;
        } else {
            fclose(file_ptr_);
            throw std::runtime_error("FileInputStream: Cannot read size header from file: " + id_);
        }
    }
    DEBUG_COUT_INFO(
        "FileInputStream: " << id_ << " opened. Header elements: " << total_elements_in_file_
                            << std::endl);

    if (total_elements_in_file_ == 0) {
        is_exhausted_ = true;
        has_valid_value_ = false;
    } else {
        Advance();
    }
}

template <typename T>
FileInputStream<T>::~FileInputStream() {
    if (file_ptr_) {
        fclose(file_ptr_);
        DEBUG_COUT_INFO("FileInputStream: Closed " << id_ << std::endl);
    }
}

template <typename T>
FileInputStream<T>::FileInputStream(FileInputStream&& other) noexcept
    : id_(std::move(other.id_))
    , file_ptr_(other.file_ptr_)
    , buffer_(std::move(other.buffer_))
    , total_elements_in_file_(other.total_elements_in_file_)
    , total_elements_read_(other.total_elements_read_)
    , is_exhausted_(other.is_exhausted_)
    , current_value_(std::move(other.current_value_))
    , has_valid_value_(other.has_valid_value_)
    , serializer_member_(std::move(other.serializer_member_)) {
    other.file_ptr_ = nullptr;
}

template <typename T>
FileInputStream<T>& FileInputStream<T>::operator=(FileInputStream&& other) noexcept {
    if (this != &other) {
        if (file_ptr_) {
            fclose(file_ptr_);
        }
        id_ = std::move(other.id_);
        file_ptr_ = other.file_ptr_;
        buffer_ = std::move(other.buffer_);
        total_elements_in_file_ = other.total_elements_in_file_;
        total_elements_read_ = other.total_elements_read_;
        is_exhausted_ = other.is_exhausted_;
        current_value_ = std::move(other.current_value_);
        has_valid_value_ = other.has_valid_value_;
        serializer_member_ = std::move(other.serializer_member_);
        other.file_ptr_ = nullptr;
    }
    return *this;
}

template <typename T>
void FileInputStream<T>::Advance() {
    if (is_exhausted_ ||
        (total_elements_in_file_ > 0 && total_elements_read_ >= total_elements_in_file_)) {
        has_valid_value_ = false;
        is_exhausted_ = true;
        return;
    }
    if (!buffer_.HasMoreToRead()) {
        FillBufferInternal();
        if (!buffer_.HasMoreToRead()) {
            has_valid_value_ = false;
            is_exhausted_ = true;
            return;
        }
    }
    current_value_ = buffer_.ReadNext();
    total_elements_read_++;
    has_valid_value_ = true;
    if (total_elements_read_ >= total_elements_in_file_) {
        is_exhausted_ = true;
    }
}

template <typename T>
const T& FileInputStream<T>::Value() const {
    if (!has_valid_value_) {
        throw std::logic_error("Value from exhausted FileInputStream: " + id_);
    }
    return current_value_;
}

template <typename T>
bool FileInputStream<T>::IsExhausted() const {
    return is_exhausted_ && !has_valid_value_;
}

template <typename T>
bool FileInputStream<T>::IsEmptyOriginalStorage() const {
    return total_elements_in_file_ == 0;
}

template <typename T>
void FileOutputStream<T>::FlushBufferInternal() {
    if (buffer_.IsEmpty() || !file_ptr_ || finalized_) {
        return;
    }

    uint64_t successful_writes = 0;
    if constexpr (PodSerializable<T>) {
        successful_writes = fwrite(buffer_.Data(), sizeof(T), buffer_.Size(), file_ptr_);
    } else {
        const T* data = buffer_.Data();
        for (uint64_t i = 0; i < buffer_.Size(); ++i) {
            if (!serializer_member_->Serialize(data[i], file_ptr_)) {
                throw std::runtime_error(
                    "FileOutputStream: Failed to Serialize element to file: " + id_);
            }
            successful_writes++;
        }
    }

    if (successful_writes != buffer_.Size()) {
        throw std::runtime_error("FileOutputStream: Failed to write full buffer to file: " + id_);
    }

    DEBUG_COUT_SUCCESS(
        "FileOutputStream: Flushed " << buffer_.Size() << " elements to " << id_ << std::endl);
    buffer_.Clear();
}

template <typename T>
FileOutputStream<T>::FileOutputStream(const StorageId& filename, uint64_t buffer_capacity_elements)
    : id_(filename), buffer_(buffer_capacity_elements) {
    if constexpr (!PodSerializable<T>) {
        serializer_member_ = create_serializer<T>();
    }
    file_ptr_ = fopen(id_.c_str(), "wb");
    if (!file_ptr_) {
        throw std::runtime_error("FileOutputStream: Cannot open output file: " + id_);
    }
    if (setvbuf(file_ptr_, nullptr, _IONBF, 0) != 0) {
        DEBUG_COUT_WARNING(
            "Warning: FileOutputStream could not disable stdio buffering for " << id_ << std::endl);
    }
    uint64_t placeholder_size = 0;
    if (fwrite(&placeholder_size, sizeof(uint64_t), 1, file_ptr_) != 1) {
        fclose(file_ptr_);
        throw std::runtime_error("FileOutputStream: Failed to write placeholder size to " + id_);
    }
    DEBUG_COUT_INFO("FileOutputStream: " << id_ << " opened for writing." << std::endl);
}

template <typename T>
FileOutputStream<T>::~FileOutputStream() {
    Finalize();
}

template <typename T>
FileOutputStream<T>::FileOutputStream(FileOutputStream&& other) noexcept
    : id_(std::move(other.id_))
    , file_ptr_(other.file_ptr_)
    , buffer_(std::move(other.buffer_))
    , total_elements_written_(other.total_elements_written_)
    , finalized_(other.finalized_)
    , serializer_member_(std::move(other.serializer_member_)) {
    other.file_ptr_ = nullptr;
    other.finalized_ = true;
}

template <typename T>
FileOutputStream<T>& FileOutputStream<T>::operator=(FileOutputStream&& other) noexcept {
    if (this != &other) {
        Finalize();
        id_ = std::move(other.id_);
        file_ptr_ = other.file_ptr_;
        buffer_ = std::move(other.buffer_);
        total_elements_written_ = other.total_elements_written_;
        finalized_ = other.finalized_;
        serializer_member_ = std::move(other.serializer_member_);
        other.file_ptr_ = nullptr;
        other.finalized_ = true;
    }
    return *this;
}

template <typename T>
void FileOutputStream<T>::Write(const T& value) {
    if (finalized_) {
        throw std::logic_error("Write to finalized FileOutputStream: " + id_);
    }
    if (buffer_.PushBack(value)) {
        FlushBufferInternal();
    }
    total_elements_written_++;
}

template <typename T>
void FileOutputStream<T>::Finalize() {
    if (finalized_ || !file_ptr_) {
        return;
    }
    FlushBufferInternal();
    if (fseek(file_ptr_, 0, SEEK_SET) != 0) {
        DEBUG_COUT_ERROR("Error: FileOutputStream fseek failed for " << id_ << std::endl);
    } else {
        if (fwrite(&total_elements_written_, sizeof(uint64_t), 1, file_ptr_) != 1) {
            DEBUG_COUT_ERROR(
                "Error: FileOutputStream fwrite header failed for " << id_ << std::endl);
        } else {
            DEBUG_COUT_SUCCESS(
                "FileOutputStream: Finalized "
                << id_ << ". Header elements: " << total_elements_written_ << std::endl);
        }
    }
    fflush(file_ptr_);
    fclose(file_ptr_);
    file_ptr_ = nullptr;
    finalized_ = true;
}

template <typename T>
uint64_t FileOutputStream<T>::GetTotalElementsWritten() const {
    return total_elements_written_;
}

template <typename T>
StorageId FileOutputStream<T>::GetId() const {
    return id_;
}

template <typename T>
FileStreamFactory<T>::FileStreamFactory(const std::string& base_temp_dir_name)
    : temp_file_manager_(base_temp_dir_name) {
}

template <typename T>
std::unique_ptr<IInputStream<T>> FileStreamFactory<T>::CreateInputStream(
    const StorageId& id, uint64_t buffer_capacity_elements) {
    return std::make_unique<FileInputStream<T>>(id, buffer_capacity_elements);
}

template <typename T>

std::unique_ptr<IOutputStream<T>> FileStreamFactory<T>::CreateOutputStream(
    const StorageId& id, uint64_t buffer_capacity_elements) {
    return std::make_unique<FileOutputStream<T>>(id, buffer_capacity_elements);
}

template <typename T>

std::unique_ptr<IOutputStream<T>> FileStreamFactory<T>::CreateTempOutputStream(
    StorageId& out_temp_id, uint64_t buffer_capacity_elements) {
    out_temp_id = temp_file_manager_.GenerateTempFilename("r", ".b");
    return std::make_unique<FileOutputStream<T>>(out_temp_id, buffer_capacity_elements);
}

template <typename T>

void FileStreamFactory<T>::DeleteStorage(const StorageId& id) {
    temp_file_manager_.CleanupFile(id);
}

template <typename T>

void FileStreamFactory<T>::MakeStoragePermanent(
    const StorageId& temp_id, const StorageId& final_id) {
    if (temp_id == final_id) {
        return;
    }
    try {
        if (std::filesystem::exists(final_id)) {
            std::filesystem::remove(final_id);
        }
        std::filesystem::rename(temp_id, final_id);
    } catch (const std::filesystem::filesystem_error& e) {
        DEBUG_COUT_WARNING(
            "MakeStoragePermanent: Rename failed ("
            << e.what() << "), attempting copy for " << temp_id << " to " << final_id << std::endl);
        {
            FileInputStream<T> src(temp_id, 1024);
            FileOutputStream<T> dst(final_id, 1024);
            while (!src.IsExhausted()) {
                dst.Write(src.Value());
                src.Advance();
            }
        }
        temp_file_manager_.CleanupFile(temp_id);
    }
}

template <typename T>

bool FileStreamFactory<T>::StorageExists(const StorageId& id) const {
    return std::filesystem::exists(id);
}

template <typename T>

StorageId FileStreamFactory<T>::GetTempStorageContextId() const {
    return temp_file_manager_.GetBaseDirPath().string();
}
