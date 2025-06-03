/**
 * @file element_buffer.hpp
 * @brief Шаблонный класс буфера для элементов
 * @author External Sort Library
 * @version 1.0
 */

#pragma once

#include <cstdint>
#include <vector>

namespace external_sort {

/**
 * @brief Шаблонный класс буфера для элементов типа T
 *
 * ElementBuffer предоставляет буферизацию в памяти для чтения или записи
 * последовательности элементов. Он управляет емкостью, текущим количеством
 * валидных элементов и курсором чтения.
 *
 * @tparam T Тип элементов, хранящихся в буфере
 */
template <typename T>
class ElementBuffer {
   private:
    std::vector<T> storage_{};      ///< Внутреннее хранилище элементов (вектор)
    uint64_t capacity_elements_{};  ///< Максимальная емкость буфера в элементах
    uint64_t
        num_valid_elements_{};  ///< Текущее количество валидных (заполненных) элементов в буфере
    uint64_t read_cursor_{};    ///< Позиция курсора для следующего чтения из буфера

   public:
    /**
     * @brief Конструктор буфера с заданной емкостью
     * @param capacity Емкость буфера в элементах (минимум 1)
     */
    explicit ElementBuffer(uint64_t capacity);

    /**
     * @brief Добавляет элемент в конец буфера
     * @param element Элемент для добавления
     * @return true, если буфер заполнился после добавления элемента
     */
    bool PushBack(const T& element);

    /**
     * @brief Возвращает указатель на данные буфера (только для чтения)
     * @return Константный указатель на начало данных
     */
    [[nodiscard]] const T* Data() const;

    /**
     * @brief Возвращает количество валидных элементов в буфере
     * @return Количество валидных элементов
     */
    [[nodiscard]] uint64_t Size() const;

    /**
     * @brief Возвращает указатель на сырые данные буфера (для записи)
     * @return Указатель на начало данных
     */
    T* RawDataPtr();

    /**
     * @brief Устанавливает количество валидных элементов в буфере
     * @param count Новое количество валидных элементов
     * @throws std::length_error если count превышает емкость буфера
     */
    void SetValidElementsCount(uint64_t count);

    /**
     * @brief Читает следующий элемент из буфера и перемещает курсор
     * @return Следующий элемент или T{} если элементов больше нет
     */
    T ReadNext();

    /**
     * @brief Проверяет, есть ли ещё элементы для чтения
     * @return true, если есть элементы для чтения
     */
    [[nodiscard]] bool HasMoreToRead() const;

    /**
     * @brief Возвращает максимальную емкость буфера
     * @return Емкость буфера в элементах
     */
    [[nodiscard]] uint64_t Capacity() const;

    /**
     * @brief Проверяет, пуст ли буфер
     * @return true, если в буфере нет валидных элементов
     */
    [[nodiscard]] bool IsEmpty() const;

    /**
     * @brief Проверяет, заполнен ли буфер
     * @return true, если буфер заполнен до максимальной емкости
     */
    [[nodiscard]] bool IsFull() const;

    /**
     * @brief Очищает буфер, сбрасывая количество валидных элементов и курсор чтения
     */
    void Clear();

    ElementBuffer(ElementBuffer&&) = default;
    ElementBuffer& operator=(ElementBuffer&&) = default;
    ElementBuffer(const ElementBuffer&) = delete;
    ElementBuffer& operator=(const ElementBuffer&) = delete;
    ~ElementBuffer() = default;
};

#include "impl/element_buffer.tpp"

}  // namespace external_sort
