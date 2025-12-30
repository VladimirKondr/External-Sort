/**
 * @file file_stream.hpp
 * @brief File implementations of input/output streams
 */

#pragma once

#include "element_buffer.hpp"
#include "interfaces.hpp"
#include "temp_file_manager.hpp"
#include "serializers.hpp"
#include "io_logging.hpp"

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <cstdio>
#include <filesystem>
#include <stdexcept>
#include <utility>

namespace io {

/**
 * @brief File implementation of an input stream
 *
 * Implements the IInputStream interface for reading data from a file.
 * Uses buffering for efficient reading.
 *
 * Note: This implementation provides efficient access for non-trivial types
 * by supporting TakeValue() (move out) and minimizing copies when possible.
 *
 * @tparam T The type of elements in the stream
 */
template <typename T>
class FileInputStream : public IInputStream<T> {
private:
    StorageId id_;                         ///< Storage identifier (file name)
    FILE* file_ptr_ = nullptr;             ///< Pointer to the file
    ElementBuffer<T> buffer_;              ///< Buffer for reading
    uint64_t total_elements_in_file_ = 0;  ///< Total number of elements in the file
    uint64_t total_elements_read_ = 0;     ///< Number of elements read so far
    bool is_exhausted_ = false;            ///< Flag indicating if the stream is exhausted
    T current_value_{};                    ///< The current element
    bool has_valid_value_ = false;         ///< Flag indicating if the current element is valid
    std::unique_ptr<serialization::Serializer<T>>
        serializer_member_{};  ///< Serializer for the elements

    /**
     * @brief Fills the internal buffer with data from the file
     */
    void FillBuffer();

public:
    /**
     * @brief Constructor for the file input stream
     * @param filename The name of the file to read from
     * @param buffer_capacity_elements Buffer capacity in elements
     * @throws std::runtime_error if the file cannot be opened
     */
    FileInputStream(const StorageId& filename, uint64_t buffer_capacity_elements);

    /**
     * @brief Destructor, closes the file
     */
    ~FileInputStream() override;

    FileInputStream(const FileInputStream&) = delete;
    FileInputStream& operator=(const FileInputStream&) = delete;

    FileInputStream(FileInputStream&& other) noexcept;
    FileInputStream& operator=(FileInputStream&& other) noexcept;

    /**
     * @brief Advances the stream to the next element
     */
    void Advance() override;

    /**
     * @brief Returns the current element of the stream
     * @return A constant reference to the current element
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
     * @return true if the file contained no elements
     */
    bool IsEmptyOriginalStorage() const override;

    /**
     * @brief Transfers the ownership of the current value and resets the state
     * @return The current value
     * @throws std::logic_error if the stream is exhausted
     */
    T TakeValue();
};

/**
 * @brief File implementation of an output stream
 *
 * Implements the IOutputStream interface for writing data to a file.
 * Uses buffering for efficient writing.
 *
 * @tparam T The type of elements in the stream
 */
template <typename T>
class FileOutputStream : public IOutputStream<T> {
private:
    StorageId id_;                         ///< Storage identifier (file name)
    FILE* file_ptr_ = nullptr;             ///< Pointer to the file
    ElementBuffer<T> buffer_;              ///< Buffer for writing
    uint64_t total_elements_written_ = 0;  ///< Number of elements written
    uint64_t total_bytes_written_ = 0;     ///< Total bytes written (data + header)
    bool finalized_ = false;               ///< Flag indicating if the stream has been finalized
    std::unique_ptr<serialization::Serializer<T>>
        serializer_member_{};  ///< Serializer for the elements

    /**
     * @brief Flushes the buffer to the file
     */
    void FlushBufferInternal();

public:
    /**
     * @brief Constructor for the file output stream
     * @param filename The name of the file to write to
     * @param buffer_capacity_elements Buffer capacity in elements
     * @throws std::runtime_error if the file cannot be created
     */
    FileOutputStream(const StorageId& filename, uint64_t buffer_capacity_elements);

    /**
     * @brief Destructor, finalizes the stream
     */
    ~FileOutputStream() override;

    FileOutputStream(const FileOutputStream&) = delete;
    FileOutputStream& operator=(const FileOutputStream&) = delete;

    FileOutputStream(FileOutputStream&& other) noexcept;
    FileOutputStream& operator=(FileOutputStream&& other) noexcept;

    /**
     * @brief Writes an element to the stream
     * @param value The element to write
     * @throws std::logic_error if the stream has been finalized
     */
    void Write(const T& value) override;

    /**
     * @brief Writes an rvalue element to the stream
     * @param value The rvalue element to write
     * @throws std::logic_error if the stream has been finalized
     */
    void Write(T&& value);

    /**
     * @brief Finalizes the stream, writing all buffered data to the file
     */
    void Finalize() override;

    /**
     * @brief Returns the total number of elements written
     * @return The number of elements written to the stream
     */
    uint64_t GetTotalElementsWritten() const override;

    /**
     * @brief Returns the total number of bytes written to the file
     * @return The total number of bytes written (including header)
     */
    uint64_t GetTotalBytesWritten() const override;

    /**
     * @brief Returns the file identifier
     * @return The StorageId of the file
     */
    StorageId GetId() const override;
};

/**
 * @brief Factory for file streams
 *
 * Implements the IStreamFactory interface for creating file-based I/O streams
 * and managing temporary files.
 *
 * @tparam T The type of elements in the streams
 */
template <typename T>
class FileStreamFactory : public IStreamFactory<T> {
private:
    TempFileManager temp_file_manager_;  ///< Temporary file manager

public:
    /**
     * @brief Constructor for the file stream factory
     * @param base_temp_dir_name The name of the base directory for temporary files
     */
    explicit FileStreamFactory(const std::string& base_temp_dir_name = "temp_files");

    /**
     * @brief Creates an input stream for reading from a file
     * @param id The identifier of the file
     * @param buffer_capacity_elements The buffer capacity
     * @return A unique pointer to an IInputStream
     */
    std::unique_ptr<IInputStream<T>> CreateInputStream(const StorageId& id,
                                                       uint64_t buffer_capacity_elements) override;

    /**
     * @brief Creates an output stream for writing to a file
     * @param id The identifier of the file
     * @param buffer_capacity_elements The buffer capacity
     * @return A unique pointer to an IOutputStream
     */
    std::unique_ptr<IOutputStream<T>> CreateOutputStream(
        const StorageId& id, uint64_t buffer_capacity_elements) override;

    /**
     * @brief Creates an output stream for writing to a temporary file
     * @param out_temp_id Output parameter for the temporary file's ID
     * @param buffer_capacity_elements The buffer capacity
     * @return A unique pointer to an IOutputStream
     */
    std::unique_ptr<IOutputStream<T>> CreateTempOutputStream(
        StorageId& out_temp_id, uint64_t buffer_capacity_elements) override;

    /**
     * @brief Deletes a file
     * @param id The identifier of the file to delete
     */
    void DeleteStorage(const StorageId& id) override;

    /**
     * @brief Makes a temporary file permanent
     * @param temp_id The identifier of the temporary file
     * @param final_id The identifier of the final file
     */
    void MakeStoragePermanent(const StorageId& temp_id, const StorageId& final_id) override;

    /**
     * @brief Checks if a file exists
     * @param id The identifier of the file
     * @return true if the file exists
     */
    bool StorageExists(const StorageId& id) const override;

    /**
     * @brief Returns the path to the temporary files directory
     * @return The path to the temporary files directory
     */
    StorageId GetTempStorageContextId() const override;
};

template <typename T>
void FileInputStream<T>::FillBuffer() {
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
    if constexpr (serialization::PodSerializable<T>) {
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
            throw std::runtime_error("FileInputStream: Error reading from file: " + id_ +
                                     " (ferror set)");
        }
        throw std::runtime_error("FileInputStream: Failed to read expected elements from file: " +
                                 id_);
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
    if constexpr (!serialization::PodSerializable<T>) {
        serializer_member_ = serialization::CreateSerializer<T>();
    }
    file_ptr_ = fopen(id_.c_str(), "rb");
    if (!file_ptr_) {
        throw std::runtime_error("FileInputStream: Cannot open input file: " + id_);
    }
    if (setvbuf(file_ptr_, nullptr, _IONBF, 0) != 0) {
        detail::LogWarning("FileInputStream could not disable stdio buffering for " + id_);
    }

    if (fread(&total_elements_in_file_, sizeof(uint64_t), 1, file_ptr_) != 1) {
        if (feof(file_ptr_) && std::filesystem::file_size(id_) < sizeof(uint64_t)) {
            total_elements_in_file_ = 0;
        } else {
            fclose(file_ptr_);
            throw std::runtime_error("FileInputStream: Cannot read size header from file: " + id_);
        }
    }
    detail::LogInfo("FileInputStream: " + id_ +
                    " opened. Header elements: " + std::to_string(total_elements_in_file_));

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
        detail::LogInfo("FileInputStream: Closed " + id_);
    }
}

template <typename T>
FileInputStream<T>::FileInputStream(FileInputStream&& other) noexcept
    : id_(std::move(other.id_)),
      file_ptr_(other.file_ptr_),
      buffer_(std::move(other.buffer_)),
      total_elements_in_file_(other.total_elements_in_file_),
      total_elements_read_(other.total_elements_read_),
      is_exhausted_(other.is_exhausted_),
      current_value_(std::move(other.current_value_)),
      has_valid_value_(other.has_valid_value_),
      serializer_member_(std::move(other.serializer_member_)) {
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
        FillBuffer();
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
T FileInputStream<T>::TakeValue() {
    if (!has_valid_value_) {
        throw std::logic_error("TakeValue from exhausted FileInputStream: " + id_);
    }
    T tmp = std::move(current_value_);
    has_valid_value_ = false;
    return tmp;
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
    if constexpr (serialization::PodSerializable<T>) {
        successful_writes = fwrite(buffer_.Data(), sizeof(T), buffer_.Size(), file_ptr_);
        total_bytes_written_ += successful_writes * sizeof(T);
    } else {
        const T* data = buffer_.Data();
        for (uint64_t i = 0; i < buffer_.Size(); ++i) {
            uint64_t element_size = serializer_member_->GetSerializedSize(data[i]);
            if (!serializer_member_->Serialize(data[i], file_ptr_)) {
                throw std::runtime_error("FileOutputStream: Failed to Serialize element to file: " +
                                         id_);
            }
            total_bytes_written_ += element_size;
            successful_writes++;
        }
    }

    if (successful_writes != buffer_.Size()) {
        throw std::runtime_error("FileOutputStream: Failed to write full buffer to file: " + id_);
    }

    detail::LogInfo("FileOutputStream: Flushed " + std::to_string(buffer_.Size()) +
                    " elements to " + id_);
    buffer_.Clear();
}

template <typename T>
FileOutputStream<T>::FileOutputStream(const StorageId& filename, uint64_t buffer_capacity_elements)
    : id_(filename), buffer_(buffer_capacity_elements) {
    if constexpr (!serialization::PodSerializable<T>) {
        serializer_member_ = serialization::CreateSerializer<T>();
    }
    file_ptr_ = fopen(id_.c_str(), "wb");
    if (!file_ptr_) {
        throw std::runtime_error("FileOutputStream: Cannot open output file: " + id_);
    }
    if (setvbuf(file_ptr_, nullptr, _IONBF, 0) != 0) {
        detail::LogWarning("FileOutputStream could not disable stdio buffering for " + id_);
    }
    uint64_t placeholder_size = 0;
    if (fwrite(&placeholder_size, sizeof(uint64_t), 1, file_ptr_) != 1) {
        fclose(file_ptr_);
        throw std::runtime_error("FileOutputStream: Failed to write placeholder size to " + id_);
    }
    // Account for the header size in total bytes written
    total_bytes_written_ = sizeof(uint64_t);
    detail::LogInfo("FileOutputStream: " + id_ + " opened for writing.");
}

template <typename T>
FileOutputStream<T>::~FileOutputStream() {
    Finalize();
}

template <typename T>
FileOutputStream<T>::FileOutputStream(FileOutputStream&& other) noexcept
    : id_(std::move(other.id_)),
      file_ptr_(other.file_ptr_),
      buffer_(std::move(other.buffer_)),
      total_elements_written_(other.total_elements_written_),
      total_bytes_written_(other.total_bytes_written_),
      finalized_(other.finalized_),
      serializer_member_(std::move(other.serializer_member_)) {
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
        total_bytes_written_ = other.total_bytes_written_;
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
void FileOutputStream<T>::Write(T&& value) {
    if (finalized_) {
        throw std::logic_error("Write to finalized FileOutputStream: " + id_);
    }
    if (buffer_.PushBack(std::move(value))) {
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
        detail::LogError("Error: FileOutputStream fseek failed for " + id_);
    } else {
        if (fwrite(&total_elements_written_, sizeof(uint64_t), 1, file_ptr_) != 1) {
            detail::LogError("Error: FileOutputStream fwrite header failed for " + id_);
        } else {
            detail::LogInfo("FileOutputStream: Finalized " + id_ +
                            ". Header elements: " + std::to_string(total_elements_written_));
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
uint64_t FileOutputStream<T>::GetTotalBytesWritten() const {
    return total_bytes_written_;
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

void FileStreamFactory<T>::MakeStoragePermanent(const StorageId& temp_id,
                                                const StorageId& final_id) {
    if (temp_id == final_id) {
        return;
    }
    try {
        if (std::filesystem::exists(final_id)) {
            std::filesystem::remove(final_id);
        }
        std::filesystem::rename(temp_id, final_id);
    } catch (const std::filesystem::filesystem_error& e) {
        detail::LogWarning("MakeStoragePermanent: Rename failed (" + std::string(e.what()) +
                           "), attempting copy for " + temp_id + " to " + final_id);
        {
            FileInputStream<T> src(temp_id, 1024);
            FileOutputStream<T> dst(final_id, 1024);
            while (!src.IsExhausted()) {
                dst.Write(src.TakeValue());
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

}  // namespace io
