/**
 * @file type_concepts.hpp
 * @brief Концепты для проверки типов данных в библиотеке внешней сортировки
 * @author External Sort Library
 * @version 1.0
 */

#pragma once

#include <concepts>
#include <cstdio>
#include <string>
#include <type_traits>
#include <vector>

namespace external_sort {

/**
 * @brief Концепт для сравнимых типов
 *
 * Проверяет, что тип поддерживает операторы сравнения, необходимые для сортировки.
 */
template <typename T>
concept Sortable = std::totally_ordered<T>;

/**
 * @brief Концепт для POD типов, которые можно сериализовать через fwrite/fread
 *
 * Проверяет, что тип является тривиально копируемым и имеет стандартную раскладку.
 */
template <typename T>
concept PodSerializable = std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>;

/**
 * @brief Концепт для типов с кастомной сериализацией через свободные функции
 *
 * Проверяет, что для типа определены свободные функции Serialize и Deserialize.
 * Эти функции должны быть доступны через ADL (аргументно-зависимый поиск),
 * что обычно означает, что они должны быть объявлены в том же пространстве
 * имен, что и сам тип.
 *
 * Функции должны иметь следующие сигнатуры:
 * - bool Serialize(const T& obj, FILE* file);
 * - bool Deserialize(T& obj, FILE* file);
 */
template <typename T>
concept CustomSerializable = requires(const T& obj, T& obj_mut, FILE* file) {
    { Serialize(obj, file) } -> std::same_as<bool>;
    { Deserialize(obj_mut, file) } -> std::same_as<bool>;
};

/**
 * @brief Концепт для типов с методами сериализации
 *
 * Проверяет, что тип имеет методы Serialize и Deserialize.
 */
template <typename T>
concept MethodSerializable = requires(const T& obj, T& obj_mut, FILE* file) {
    { obj.Serialize(file) } -> std::same_as<bool>;
    { obj_mut.Deserialize(file) } -> std::same_as<bool>;
};

/**
 * @brief Концепт для типов со специальной поддержкой сериализации
 *
 * Включает типы, для которых есть специализации Serializer
 */
template <typename T>
concept SpecializedSerializable =
    std::same_as<T, std::string> || requires { typename std::vector<typename T::value_type>; };

/**
 * @brief Концепт для типов, которые можно использовать в файловых потоках
 *
 * Тип должен быть либо POD, либо иметь какую-либо форму сериализации, либо иметь специализацию.
 */
template <typename T>
concept FileSerializable = PodSerializable<T> || CustomSerializable<T> || MethodSerializable<T> ||
                           SpecializedSerializable<T>;

/**
 * @brief Концепт для типов, поддерживаемых библиотекой
 *
 * Тип должен быть сравнимым и, для файловых операций, сериализуемым.
 */
template <typename T>
concept SupportedType = Sortable<T>;

/**
 * @brief Концепт для типов, поддерживаемых в файловых операциях
 */
template <typename T>
concept SupportedFileType = SupportedType<T> && FileSerializable<T>;

}  // namespace external_sort
