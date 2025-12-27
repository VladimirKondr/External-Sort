/**
 * @file serializers.hpp
 * @brief Serializers implementations for some types
 *
 * @section adding_serialization Adding Serialization Support
 *
 * **For std:: or external namespace types:**
 *
 * 1. Add serialization::Serializer specialization in `type_concepts.hpp`:
 * @code{.cpp}
 * template <>
 * class Serializer<std::string> {
 * public:
 *     using Specialized = std::true_type;
 * 
 *     bool Serialize(const std::string& obj, FILE* file) {
 *         ...
 *     }
 * 
 *     bool Deserialize(std::string& obj, FILE* file) {
 *         ...
 *     }
 * };
 * @endcode
 *
 * **Alternative:** Create a wrapper type with Serialize/Deserialize methods
 * or Serialize/Deserialize free functions in its' namespace.
 *
 * @see serialization::CreateSerializer
 * @see serialization::Serializer<std::vector<T>>
 * @see type_concepts.hpp for serialization concepts
 */

#pragma once

#include "type_concepts.hpp"

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace serialization {

/**
 * @brief Base interface for serializable types
 */
template <typename T>
class Serializer {
public:
    using Specialized = std::false_type;

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
 * @brief Serializer for types with free function serialization
 *
 * @see serialization::CustomSerializable
 */
template <CustomSerializable T>
class CustomFunctionSerializer : public Serializer<T> {
public:
    bool Serialize(const T& obj, FILE* file) override {
        bool result = detail_adl::AdlSerialize(obj, file);
        return result && (ferror(file) == 0);
    }

    bool Deserialize(T& obj, FILE* file) override {
        bool result = detail_adl::AdlDeserialize(obj, file);
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
    } else if constexpr (SpecializedSerializable<T>) {
        return std::make_unique<Serializer<T>>();
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

template <>
class Serializer<std::string> {
public:
    using Specialized = std::true_type;
    bool Serialize(const std::string& obj, FILE* file) {
        uint64_t length = obj.length();
        if (fwrite(&length, sizeof(uint64_t), 1, file) != 1) {
            return false;
        }
        return fwrite(obj.data(), sizeof(char), length, file) == length;
    }

    bool Deserialize(std::string& obj, FILE* file) {
        uint64_t length;
        if (fread(&length, sizeof(uint64_t), 1, file) != 1) {
            return false;
        }
        obj.resize(length);
        return fread(&obj[0], sizeof(char), length, file) == length;
    }
};

template <typename T>
class Serializer<std::vector<T>> {
public:
    using Specialized = std::true_type;

    bool Serialize(const std::vector<T>& obj, FILE* file) {
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

    bool Deserialize(std::vector<T>& obj, FILE* file) {
        uint64_t size;
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
};

}  // namespace serialization
