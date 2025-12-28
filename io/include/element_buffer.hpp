/**
 * @file element_buffer.hpp
 * @brief Element buffer template class
 * @version 1.0
 */

#pragma once

#include <cstdint>
#include <stdexcept>
#include <vector>

namespace io {

/**
 * @brief A template class for a buffer of elements of type T
 *
 * ElementBuffer provides in-memory buffering for reading or writing
 * a sequence of elements. It manages the capacity, the current number
 * of valid elements, and the read cursor.
 *
 * @tparam T The type of elements stored in the buffer
 */
template <typename T>
class ElementBuffer {
private:
    std::vector<T> storage_{};       ///< Internal storage for elements
    uint64_t capacity_elements_{};   ///< Maximum buffer capacity in elements
    uint64_t num_valid_elements_{};  ///< Current number of valid (filled) elements in the buffer
    uint64_t read_cursor_{};         ///< Cursor position for the next read from the buffer

public:
    /**
     * @brief Constructs a buffer with a given capacity
     * @param capacity The buffer capacity in elements (minimum 1)
     */
    explicit ElementBuffer(uint64_t capacity);

    /**
     * @brief Adds an element to the end of the buffer
     * @param element The element to add
     * @return true if the buffer becomes full after adding the element
     */
    bool PushBack(const T& element);

    /**
     * @brief Returns a pointer to the buffer's data (read-only)
     * @return A const pointer to the beginning of the data
     */
    [[nodiscard]] const T* Data() const;

    /**
     * @brief Returns the number of valid elements in the buffer
     * @return The number of valid elements
     */
    [[nodiscard]] uint64_t Size() const;

    /**
     * @brief Returns a pointer to the raw buffer data (for writing)
     * @return A pointer to the beginning of the data
     */
    T* RawDataPtr();

    /**
     * @brief Sets the number of valid elements in the buffer
     * @param count The new number of valid elements
     * @throws std::length_error if count exceeds the buffer's capacity
     */
    void SetValidElementsCount(uint64_t count);

    /**
     * @brief Reads the next element from the buffer and advances the cursor
     * @return The next element, or T{} if there are no more elements
     */
    T ReadNext();

    /**
     * @brief Checks if there are more elements to read
     * @return true if there are more elements to read
     */
    [[nodiscard]] bool HasMoreToRead() const;

    /**
     * @brief Returns the maximum capacity of the buffer
     * @return The buffer capacity in elements
     */
    [[nodiscard]] uint64_t Capacity() const;

    /**
     * @brief Checks if the buffer is empty
     * @return true if there are no valid elements in the buffer
     */
    [[nodiscard]] bool IsEmpty() const;

    /**
     * @brief Checks if the buffer is full
     * @return true if the buffer is filled to its maximum capacity
     */
    [[nodiscard]] bool IsFull() const;

    /**
     * @brief Clears the buffer, resetting the count of valid elements and the read cursor
     */
    void Clear();

    ElementBuffer(ElementBuffer&&) = default;
    ElementBuffer& operator=(ElementBuffer&&) = default;
    ElementBuffer(const ElementBuffer&) = delete;
    ElementBuffer& operator=(const ElementBuffer&) = delete;
    ~ElementBuffer() = default;
};

}  // namespace io

namespace io {

template <typename T>
ElementBuffer<T>::ElementBuffer(uint64_t capacity)
    : capacity_elements_(std::max(static_cast<uint64_t>(1), capacity)),
      num_valid_elements_(0),
      read_cursor_(0) {
    storage_.resize(capacity_elements_);
}

template <typename T>
bool ElementBuffer<T>::PushBack(const T& element) {
    if (num_valid_elements_ < capacity_elements_) {
        storage_[num_valid_elements_] = element;
        num_valid_elements_++;
        return num_valid_elements_ == capacity_elements_;
    }
    return true;
}

template <typename T>
const T* ElementBuffer<T>::Data() const {
    return storage_.data();
}

template <typename T>
uint64_t ElementBuffer<T>::Size() const {
    return num_valid_elements_;
}

template <typename T>
T* ElementBuffer<T>::RawDataPtr() {
    return storage_.data();
}

template <typename T>
void ElementBuffer<T>::SetValidElementsCount(uint64_t count) {
    if (count > capacity_elements_) {
        throw std::length_error("ElementBuffer: Count exceeds capacity.");
    }
    num_valid_elements_ = count;
    read_cursor_ = 0;
}

template <typename T>
T ElementBuffer<T>::ReadNext() {
    if (read_cursor_ < num_valid_elements_) {
        return storage_[read_cursor_++];
    }
    return T{};
}

template <typename T>
bool ElementBuffer<T>::HasMoreToRead() const {
    return read_cursor_ < num_valid_elements_;
}

template <typename T>
uint64_t ElementBuffer<T>::Capacity() const {
    return capacity_elements_;
}

template <typename T>
bool ElementBuffer<T>::IsEmpty() const {
    return num_valid_elements_ == 0;
}

template <typename T>
bool ElementBuffer<T>::IsFull() const {
    return num_valid_elements_ == capacity_elements_;
}

template <typename T>
void ElementBuffer<T>::Clear() {
    num_valid_elements_ = 0;
    read_cursor_ = 0;
}

}  // namespace io
