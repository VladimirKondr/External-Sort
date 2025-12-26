/**
 * @file type_concepts.hpp
 * @brief Concepts for checking types in external sort library
 */

#pragma once

#include <concepts>
#include <cstdio>
#include <type_traits>

namespace external_sort {

/**
 * @brief Concept for comparable types
 *
 * Checks that type supports compare operations, so it can be sorted.
 */
template <typename T>
concept Sortable = std::totally_ordered<T>;

/**
 * @brief Concept for POD types that can be serialized through fwrite/fread
 *
 * Checks that type is trivially copyable and has standard layout.
 * Such types can be directly written to/read from files using binary I/O.
 * 
 * Examples: int, double, struct with POD members.
 * 
 * @see external_sort::FileSerializable
 * @see external_sort::CustomSerializable
 * @see external_sort::MethodSerializable
 */
template <typename T>
concept PodSerializable = std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>;

/**
 * @brief Concept for types with custom serialization through free functions
 *
 * Checks that type has free functions Serialize and Deserialize.
 * These functions should be defined in the same namespace as the type for ADL
 * (Argument-Dependent Lookup) to work.
 *
 * Functions must have the following signatures:
 * @code{.cpp}
 * bool Serialize(const T& obj, FILE* file);
 * bool Deserialize(T& obj, FILE* file);
 * @endcode
 * 
 * - Serialize writes obj into file and returns true on success
 * - Deserialize reads one object from file into obj and returns true on success
 * 
 * @see external_sort::FileSerializable
 * @see external_sort::PodSerializable
 * @see external_sort::MethodSerializable
 */
template <typename T>
concept CustomSerializable = requires(const T& obj, T& obj_mut, FILE* file) {
    { Serialize(obj, file) } -> std::same_as<bool>;
    { Deserialize(obj_mut, file) } -> std::same_as<bool>;
};

/**
 * @brief Concept for types with serialization methods
 *
 * Checks that type has member methods Serialize and Deserialize.
 *
 * Methods must have the following signatures:
 * @code{.cpp}
 * bool Serialize(FILE* file) const;
 * bool Deserialize(FILE* file);
 * @endcode
 * 
 * - Serialize writes the object into file and returns true on success
 * - Deserialize reads one object from file and returns true on success
 * 
 * @see external_sort::FileSerializable
 * @see external_sort::PodSerializable
 * @see external_sort::CustomSerializable
 */
template <typename T>
concept MethodSerializable = requires(const T& obj, T& obj_mut, FILE* file) {
    { obj.Serialize(file) } -> std::same_as<bool>;
    { obj_mut.Deserialize(file) } -> std::same_as<bool>;
};

/**
 * @brief Concept for types that can be serialized into files.
 *
 * Types must satisfy one of the following concepts:
 * - external_sort::PodSerializable - POD types with fwrite/fread
 * - external_sort::CustomSerializable - types with free Serialize/Deserialize functions
 * - external_sort::MethodSerializable - types with Serialize/Deserialize methods
 * 
 * @see external_sort::PodSerializable
 * @see external_sort::CustomSerializable  
 * @see external_sort::MethodSerializable
 */
template <typename T>
concept FileSerializable = PodSerializable<T> || CustomSerializable<T> || MethodSerializable<T>;

/**
 * @brief Concept for supported types in the library
 *
 * Type must be sortable (satisfy external_sort::Sortable concept).
 * For file operations, type must also be serializable.
 * 
 * @see external_sort::Sortable
 * @see external_sort::SupportedFileType
 */
template <typename T>
concept SupportedType = Sortable<T>;

/**
 * @brief Concept for types supported in file operations
 * 
 * Type must satisfy both external_sort::SupportedType and external_sort::FileSerializable.
 * 
 * @see external_sort::SupportedType
 * @see external_sort::FileSerializable
 */
template <typename T>
concept SupportedFileType = SupportedType<T> && FileSerializable<T>;

}  // namespace external_sort
