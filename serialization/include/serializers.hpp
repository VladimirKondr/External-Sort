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
 *     
 *     uint64_t GetSerializedSize(const std::string& obj) {
 *         return sizeof(uint64_t) + obj.length();
 *     }
 * };
 * @endcode
 *
 * **Alternative:** Create a wrapper type with Serialize/Deserialize methods
 * or Serialize/Deserialize free functions in its' namespace.
 *
 * @section performance_optimization Performance Optimization
 *
 * The serialization library provides automatic size calculation through GetSerializedSize().
 * For optimal performance, especially in benchmarks or large-scale data processing:
 *
 * **For types using MethodSerializer (most common):**
 * Add a GetSerializedSize() const method to your type. This enables fast O(1) size
 * calculation instead of O(n) serialization to /dev/null:
 *
 * @code{.cpp}
 * struct Person {
 *     std::string name;
 *     int32_t age;
 *     std::string address;
 *     
 *     bool Serialize(FILE* file) const { ... }
 *     bool Deserialize(FILE* file) { ... }
 *     
 *     // RECOMMENDED: Add this for optimal performance!
 *     uint64_t GetSerializedSize() const {
 *         return sizeof(size_t) + name.size()      // name field
 *              + sizeof(int32_t)                   // age field
 *              + sizeof(size_t) + address.size();  // address field
 *     }
 * };
 * @endcode
 *
 * Without GetSerializedSize(), the library will work correctly but will serialize
 * to /dev/null for size calculation, which is significantly slower.
 *
 * @see serialization::CreateSerializer
 * @see serialization::Serializer<std::vector<T>>
 * @see type_concepts.hpp for serialization concepts
 */

#pragma once

#include "serialization_logging.hpp"
#include "type_concepts.hpp"

#include <cstdio>
#include <memory>
#include <string>
#include <vector>


namespace serialization {

namespace detail {

/**
 * @brief Helper class to calculate serialized size by writing to a null device
 * 
 * This class provides a fallback mechanism to calculate serialized size
 * when the type doesn't provide GetSerializedSize() method.
 * It works by actually serializing the object and counting bytes written.
 */
class ByteCountingFile {
private:
    FILE* file_;
    uint64_t bytes_written_ = 0;
    
public:
    ByteCountingFile() {
#ifdef _WIN32
        file_ = fopen("NUL", "wb");
#else
        file_ = fopen("/dev/null", "wb");
#endif
        if (!file_) {
            throw std::runtime_error("Failed to open null device for byte counting");
        }
    }
    
    ~ByteCountingFile() {
        if (file_) {
            fclose(file_);
        }
    }
    
    FILE* GetFile() { return file_; }
    
    void TrackWrite(size_t bytes) {
        bytes_written_ += bytes;
    }
    
    uint64_t GetBytesWritten() const { return bytes_written_; }
    
    ByteCountingFile(const ByteCountingFile&) = delete;
    ByteCountingFile& operator=(const ByteCountingFile&) = delete;
};

/**
 * @brief Calculate serialized size by actually serializing to /dev/null
 * 
 * This is a fallback mechanism that works for any serializable type.
 * It temporarily serializes the object to count the bytes.
 * 
 * **Performance Warning:** This function involves actual I/O operations to /dev/null.
 * While /dev/null is optimized by the OS, this is still slower than calculating
 * size arithmetically. Use this only when no faster alternative is available.
 * 
 * @tparam T Type to serialize
 * @tparam SerializeFunc Function type that performs serialization
 * @param obj Object to calculate size for
 * @param serialize_func Function that performs serialization (signature: bool(const T&, FILE*))
 * @return Number of bytes that would be written
 * @throws std::runtime_error if /dev/null cannot be opened or serialization fails
 */
template <typename T, typename SerializeFunc>
uint64_t CalculateSizeBySerializing(const T& obj, SerializeFunc serialize_func) {
    // Save current position if we're using a real file
    FILE* null_file = nullptr;
#ifdef _WIN32
    null_file = fopen("NUL", "wb");
#else
    null_file = fopen("/dev/null", "wb");
#endif
    
    if (!null_file) {
        throw std::runtime_error("Failed to open null device for size calculation");
    }
    
    // Get initial position
    int64_t start_pos = ftell(null_file);
    
    // Serialize the object
    bool result = serialize_func(obj, null_file);
    
    // Get final position
    int64_t end_pos = ftell(null_file);
    fclose(null_file);
    
    if (!result) {
        throw std::runtime_error("Failed to serialize object for size calculation");
    }
    
    // Return the number of bytes written
    return static_cast<uint64_t>(end_pos - start_pos);
}

}  // namespace detail

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

    /**
     * @brief Calculate the serialized size of an object in bytes
     *
     * This method computes the exact number of bytes that would be written
     * to a file when serializing the given object. This is useful for:
     * - Estimating storage requirements before serialization
     * - Tracking actual data size for complex types (e.g., types with dynamic memory)
     * - Generating correct amount of data to match target file sizes in benchmarks
     *
     * @param obj Object to calculate serialized size for
     * @return Number of bytes that would be written when serializing this object
     *
     * @note For POD types, this returns sizeof(T). For complex types (std::string,
     *       std::vector, custom types), this computes the actual serialized size
     *       including all dynamic data.
     *
     * @par Performance Optimization for Custom Types:
     * For types using MethodSerializer (types with Serialize/Deserialize methods),
     * there are two implementation paths:
     * 
     * 1. **Optimized path (RECOMMENDED)**: If your type provides a `GetSerializedSize()` 
     *    const method, it will be called directly. This is **much faster** as it only 
     *    calculates the size without actual serialization.
     * 
     * 2. **Fallback path**: If `GetSerializedSize()` is not provided, the serializer
     *    will perform actual serialization to /dev/null and count bytes. This works
     *    but is **significantly slower** (involves I/O operations).
     *
     * @par Example (Basic usage):
     * @code
     * auto serializer = serialization::CreateSerializer<Person>();
     * Person p{"Alice", 30, 170.5, 65, "123 Main St"};
     * uint64_t size = serializer->GetSerializedSize(p);
     * // size includes: sizeof(name length) + name data + age + height + weight + 
     * //                sizeof(address length) + address data
     * @endcode
     *
     * @par Example (Optimized custom type):
     * @code
     * struct Person {
     *     std::string name;
     *     int32_t age;
     *     std::string address;
     *     
     *     bool Serialize(FILE* file) const { ... }
     *     bool Deserialize(FILE* file) { ... }
     *     
     *     // Add this method for optimal performance!
     *     uint64_t GetSerializedSize() const {
     *         return sizeof(size_t) + name.size()
     *              + sizeof(int32_t)
     *              + sizeof(size_t) + address.size();
     *     }
     * };
     * @endcode
     *
     * @warning If your type is used in performance-critical code (e.g., benchmarks,
     *          large datasets), strongly consider implementing GetSerializedSize()
     *          method to avoid I/O overhead.
     */
    virtual uint64_t GetSerializedSize(const T& obj) = 0;
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
        bool result = fwrite(&obj, sizeof(T), 1, file) == 1;
        if (!result) {
            detail::LogError("Failed to serialize POD type of size " + std::to_string(sizeof(T)));
        }
        return result;
    }

    bool Deserialize(T& obj, FILE* file) override {
        bool result = fread(&obj, sizeof(T), 1, file) == 1;
        if (!result) {
            detail::LogError("Failed to deserialize POD type of size " + std::to_string(sizeof(T)));
        }
        return result;
    }

    uint64_t GetSerializedSize([[maybe_unused]] const T& obj) override {
        return sizeof(T);
    }
};

/**
 * @brief Serializer for types with free function serialization
 *
 * This serializer is used for types that provide Serialize() and Deserialize()
 * as free functions found via ADL (Argument Dependent Lookup).
 *
 * **Size Calculation:**
 * GetSerializedSize() performs actual serialization to /dev/null to count bytes.
 * This works reliably but involves I/O overhead.
 *
 * @par Performance Note:
 * If you need optimal performance for size calculation, consider using
 * MethodSerializable instead (with member functions) and adding a GetSerializedSize()
 * method, or create a specialized Serializer<YourType> with optimized GetSerializedSize().
 *
 * @tparam T Type with free Serialize() and Deserialize() functions
 * @see serialization::CustomSerializable
 */
template <CustomSerializable T>
class CustomFunctionSerializer : public Serializer<T> {
public:
    bool Serialize(const T& obj, FILE* file) override {
        bool result = detail_adl::AdlSerialize(obj, file);
        result = result && (ferror(file) == 0);
        if (!result) {
            detail::LogError("Custom serialization failed for type");
        }
        return result;
    }

    bool Deserialize(T& obj, FILE* file) override {
        bool result = detail_adl::AdlDeserialize(obj, file);
        result = result && (ferror(file) == 0) && (feof(file) == 0);
        if (!result) {
            detail::LogError("Custom deserialization failed for type");
        }
        return result;
    }

    uint64_t GetSerializedSize([[maybe_unused]] const T& obj) override {
        // For custom serialization, try to calculate size by actually serializing
        // This works as a fallback when specialized GetSerializedSize is not available
        return detail::CalculateSizeBySerializing(obj, [this](const T& o, FILE* f) {
            return detail_adl::AdlSerialize(o, f);
        });
    }
};

/**
 * @brief Serializer for types with method serialization
 *
 * This serializer is used for types that provide Serialize() and Deserialize()
 * methods as member functions. It supports two performance modes for size calculation:
 *
 * **Optimized Mode (Recommended):**
 * If the type provides a `GetSerializedSize() const` method returning uint64_t,
 * it will be called directly for fast size calculation without I/O overhead.
 *
 * **Fallback Mode:**
 * If GetSerializedSize() is not provided, the serializer will perform actual
 * serialization to /dev/null to count bytes. This works but is slower.
 *
 * @par Performance Tip:
 * For best performance, especially in benchmarks or when processing large datasets,
 * add a GetSerializedSize() method to your type:
 * @code
 * struct MyType {
 *     bool Serialize(FILE* file) const { ... }
 *     bool Deserialize(FILE* file) { ... }
 *     
 *     // Add this for optimal performance:
 *     uint64_t GetSerializedSize() const {
 *         return ...; // Calculate total size
 *     }
 * };
 * @endcode
 *
 * @tparam T Type with Serialize() and Deserialize() member methods
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

    uint64_t GetSerializedSize([[maybe_unused]] const T& obj) override {
        // For method-based serialization, first check if the type provides GetSerializedSize() method
        if constexpr (requires(const T& t) { { t.GetSerializedSize() } -> std::convertible_to<uint64_t>; }) {
            return obj.GetSerializedSize();
        } else {
            // Fallback: calculate size by actually serializing to /dev/null
            return detail::CalculateSizeBySerializing(obj, [](const T& o, FILE* f) {
                return o.Serialize(f);
            });
        }
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

template <>
class Serializer<std::string> {
public:
    using Specialized = std::true_type;
    bool Serialize(const std::string& obj, FILE* file) {
        uint64_t length = obj.length();
        if (fwrite(&length, sizeof(uint64_t), 1, file) != 1) {
            detail::LogError("Failed to write string length: " + std::to_string(length));
            return false;
        }
        if (fwrite(obj.data(), sizeof(char), length, file) != length) {
            detail::LogError("Failed to write string data of length: " + std::to_string(length));
            return false;
        }
        return true;
    }

    bool Deserialize(std::string& obj, FILE* file) {
        uint64_t length;
        if (fread(&length, sizeof(uint64_t), 1, file) != 1) {
            detail::LogError("Failed to read string length");
            return false;
        }
        obj.resize(length);
        if (fread(&obj[0], sizeof(char), length, file) != length) {
            detail::LogError("Failed to read string data of length: " + std::to_string(length));
            return false;
        }
        return true;
    }

    uint64_t GetSerializedSize(const std::string& obj) {
        // Size = sizeof(length field) + actual string data
        return sizeof(uint64_t) + obj.length() * sizeof(char);
    }
};

template <typename T>
class Serializer<std::vector<T>> {
public:
    using Specialized = std::true_type;

    bool Serialize(const std::vector<T>& obj, FILE* file) {
        uint64_t size = obj.size();
        if (fwrite(&size, sizeof(uint64_t), 1, file) != 1) {
            detail::LogError("Failed to write vector size: " + std::to_string(size));
            return false;
        }

        auto item_serializer = CreateSerializer<T>();
        for (size_t i = 0; i < obj.size(); ++i) {
            if (!item_serializer->Serialize(obj[i], file)) {
                detail::LogError("Failed to serialize vector element at index: " + std::to_string(i));
                return false;
            }
        }
        return true;
    }

    bool Deserialize(std::vector<T>& obj, FILE* file) {
        uint64_t size;
        if (fread(&size, sizeof(uint64_t), 1, file) != 1) {
            detail::LogError("Failed to read vector size");
            return false;
        }

        obj.resize(size);
        auto item_serializer = CreateSerializer<T>();
        for (size_t i = 0; i < size; ++i) {
            if (!item_serializer->Deserialize(obj[i], file)) {
                detail::LogError("Failed to deserialize vector element at index: " + std::to_string(i));
                return false;
            }
        }
        return true;
    }

    uint64_t GetSerializedSize(const std::vector<T>& obj) {
        // Size = sizeof(size field) + sum of element sizes
        uint64_t total_size = sizeof(uint64_t);
        auto item_serializer = CreateSerializer<T>();
        for (const auto& item : obj) {
            total_size += item_serializer->GetSerializedSize(item);
        }
        return total_size;
    }
};

}  // namespace serialization
