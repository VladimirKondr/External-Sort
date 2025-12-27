/**
 * @file type_concepts.hpp
 * @brief Concepts for checking types in serialization library
 * @author Serialization Library
 */

#pragma once

#include <concepts>
#include <cstdio>
#include <type_traits>

namespace serialization {

// Forward declaration of Serializer
template <typename T>
class Serializer;

/**
 * @brief Concept for POD types that can be serialized through fwrite/fread
 *
 * Checks that type is trivially copyable and has standard layout.
 * Such types can be directly written to/read from files using binary I/O.
 *
 * Examples: int, double, struct with POD members.
 *
 * @see serialization::FileSerializable
 * @see serialization::CustomSerializable
 * @see serialization::MethodSerializable
 */
template <typename T>
concept PodSerializable = std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>;

/**
 * @brief Helpers for ADL - defined in a nested namespace
 * 
 * These functions use unqualified lookup which enables ADL.
 * They will find Serialize/Deserialize either:
 * - In the namespace of the type T (via ADL)
 * - In the serialization namespace (via normal lookup)
 */
namespace detail_adl {
template <typename T>
auto AdlSerialize(const T& obj, FILE* file) -> decltype(Serialize(obj, file)) {
    return Serialize(obj, file);
}

template <typename T>
auto AdlDeserialize(T& obj, FILE* file) -> decltype(Deserialize(obj, file)) {
    return Deserialize(obj, file);
}
}  // namespace detail_adl

/**
 * @brief Concept for types with custom serialization through free functions
 *
 * Checks that type has free functions Serialize and Deserialize.
 * These functions can be defined either:
 * 1. In the serialization namespace (for std types like std::string, std::vector)
 * 2. In the same namespace as the type for ADL (Argument-Dependent Lookup)
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
 * @see serialization::FileSerializable
 * @see serialization::PodSerializable
 * @see serialization::MethodSerializable
 */
template <typename T>
concept CustomSerializable = requires(const T& obj, T& obj_mut, FILE* file) {
    { detail_adl::AdlSerialize(obj, file) } -> std::same_as<bool>;
    { detail_adl::AdlDeserialize(obj_mut, file) } -> std::same_as<bool>;
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
 * @see serialization::FileSerializable
 * @see serialization::PodSerializable
 * @see serialization::CustomSerializable
 */
template <typename T>
concept MethodSerializable = requires(const T& obj, T& obj_mut, FILE* file) {
    { obj.Serialize(file) } -> std::same_as<bool>;
    { obj_mut.Deserialize(file) } -> std::same_as<bool>;
};

/**
 * @brief Concept for types with Serializer specialization
 *
 * Checks that type has a specialization of class Serializer
 *
 * Specialization must define
 * @code{.cpp}
 * using Specialized = std::true_type;
 * @endcode
 *
 * @see serialization::Serializer
 * @see serialization::Serializer<std::string>
 */
template <typename T>
concept SpecializedSerializable = Serializer<T>::Specialized::value;

/**
 * @brief Concept for types that can be serialized into files.
 *
 * Types must satisfy one of the following concepts:
 * - serialization::PodSerializable - POD types with fwrite/fread
 * - serialization::CustomSerializable - types with free Serialize/Deserialize functions
 * - serialization::MethodSerializable - types with Serialize/Deserialize methods
 *
 * @see serialization::PodSerializable
 * @see serialization::CustomSerializable
 * @see serialization::MethodSerializable
 */
template <typename T>
concept FileSerializable = PodSerializable<T> || CustomSerializable<T> || MethodSerializable<T> || SpecializedSerializable<T>;

}  // namespace serialization
