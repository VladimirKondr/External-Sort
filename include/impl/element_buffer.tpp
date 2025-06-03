/**
 * @file element_buffer.tpp
 * @brief Реализация шаблонного класса ElementBuffer
 * @author External Sort Library
 * @version 1.0
 */

#pragma once

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <vector>

template <typename T>
ElementBuffer<T>::ElementBuffer(uint64_t capacity)
    : capacity_elements_(std::max(static_cast<uint64_t>(1), capacity))
    , num_valid_elements_(0)
    , read_cursor_(0) {
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