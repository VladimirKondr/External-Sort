/**
 * @file serializers.hpp
 * @brief Serializers implementations for some types
 * @author Serialization Library
 */

#pragma once

#include "type_concepts.hpp"

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace serialization::detail {

/**
 * @brief Base interface for serializable types
 */
template <typename T>
class Serializer {
   public:
    virtual ~Serializer() = default;

    Serializer() = default;
    Serializer(const Serializer&) = default;
    Serializer& operator=(const Serializer&) = default;
    Serializer(Serializer&&) = default;
    Serializer& operator=(Serializer&&) = default;

    /**
     * @brief Serialize object into file
     *
     * @param obj Object ro serialize
     * @param file Pointer to file to write to
     * @return true, if serialization is successful
     */
    virtual bool Serialize(const T& obj, FILE* file) = 0;

    /**
     * @brief Deserialize object from file
     *
     * @param obj Object to deserialize to
     * @param file Pointer to file to read from
     * @return true, if deserialization is successful
     */
    virtual bool Deserialize(T& obj, FILE* file) = 0;
};

/**
 * @brief Serializer for POD types
 *
 * @see serialization::PodSerializable
 */
template <PodSerializable T>
class PodSerializer : public Serializer<T> {
   public:
    bool Serialize(const T& obj, FILE* file) override {
        return fwrite(&obj, sizeof(T), 1, file) == 1;
    }

    bool Deserialize(T& obj, FILE* file) override {
        return fread(&obj, sizeof(T), 1, file) == 1;
    }
};

/**
 * @brief Helpers for ADL
 */

template <typename T>
bool AdlSerialize(const T& obj, FILE* file) {
    return Serialize(obj, file);
}

template <typename T>
bool AdlDeserialize(T& obj, FILE* file) {
    return Deserialize(obj, file);
}

/**
 * @brief Serializer for types with free function serialization
 *
 * @see serialization::CustomSerializable
 */
template <CustomSerializable T>
class CustomFunctionSerializer : public Serializer<T> {
   public:
    bool Serialize(const T& obj, FILE* file) override {
        bool result = AdlSerialize(obj, file);
        return result && (ferror(file) == 0);
    }

    bool Deserialize(T& obj, FILE* file) override {
        bool result = AdlDeserialize(obj, file);
        return result && (ferror(file) == 0) && (feof(file) == 0);
    }
};

/**
 * @brief Serializer for types with method serialization
 *
 * @see serialization::MethodSerializable
 */
template <MethodSerializable T>
class MethodSerializer : public Serializer<T> {
   public:
    bool Serialize(const T& obj, FILE* file) override {
        return obj.Serialize(file);
    }

    bool Deserialize(T& obj, FILE* file) override {
        return obj.Deserialize(file);
    }
};

/**
 * @brief Serializer factory
 *
 * Creates appropriate serializer based on type traits
 */
template <typename T>
std::unique_ptr<Serializer<T>> CreateSerializer() {
    static_assert(FileSerializable<T>, "Type must be serializable to be used with this library");
    if constexpr (PodSerializable<T>) {
        return std::make_unique<PodSerializer<T>>();
    } else if constexpr (CustomSerializable<T>) {
        return std::make_unique<CustomFunctionSerializer<T>>();
    } else if constexpr (MethodSerializable<T>) {
        return std::make_unique<MethodSerializer<T>>();
    } else {
        throw std::logic_error("No serializer found for type " + std::string(typeid(T).name()));
    }
}

/**
 * @brief Serialization for std::string
 */

inline bool Serialize(const std::string& obj, FILE* file) {
    uint64_t length = obj.length();
    if (fwrite(&length, sizeof(uint64_t), 1, file) != 1) {
        return false;
    }
    return fwrite(obj.data(), sizeof(char), length, file) == length;
}

inline bool Deserialize(std::string& obj, FILE* file) {
    uint64_t length = 0;
    if (fread(&length, sizeof(uint64_t), 1, file) != 1) {
        return false;
    }
    obj.resize(length);
    return fread(obj.data(), sizeof(char), length, file) == length;
}

/**
 * @brief Serialization for std::vector
 */
template <typename T>
inline bool Serialize(const std::vector<T>& obj, FILE* file) {
    uint64_t size = obj.size();
    if (fwrite(&size, sizeof(uint64_t), 1, file) != 1) {
        return false;
    }

    auto item_serializer = CreateSerializer<T>();
    for (const auto& item : obj) {
        if (!item_serializer->Serialize(item, file)) {
            return false;
        }
    }
    return true;
}

template <typename T>
inline bool Deserialize(std::vector<T>& obj, FILE* file) {
    uint64_t size = 0;
    if (fread(&size, sizeof(uint64_t), 1, file) != 1) {
        return false;
    }

    obj.resize(size);
    auto item_serializer = CreateSerializer<T>();
    for (auto& item : obj) {
        if (!item_serializer->Deserialize(item, file)) {
            return false;
        }
    }
    return true;
}

}  // namespace serialization::detail
