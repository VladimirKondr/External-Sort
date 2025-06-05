/**
 * @file serializers.hpp
 * @brief Реализация сериализаторов для различных типов данных
 * @author External Sort Library
 * @version 1.0
 */

#pragma once

#include "type_concepts.hpp"

#include <cstdio>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace external_sort {

/**
 * @brief Базовый интерфейс для сериализаторов
 *
 * Предоставляет общий интерфейс для всех сериализаторов
 */
template <typename T>
class Serializer {
   public:
    virtual ~Serializer() = default;

    /**
     * @brief Сериализовать объект в файл
     *
     * @param obj Объект для сериализации
     * @param file Указатель на файл для записи
     * @return true, если сериализация прошла успешно
     */
    virtual bool Serialize(const T& obj, FILE* file) = 0;

    /**
     * @brief Десериализовать объект из файла
     *
     * @param obj Объект для заполнения десериализованными данными
     * @param file Указатель на файл для чтения
     * @return true, если десериализация прошла успешно
     */
    virtual bool Deserialize(T& obj, FILE* file) = 0;
};

/**
 * @brief Сериализатор для POD-типов
 *
 * Использует прямую запись в память для сериализации POD-типов
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

namespace detail {
template <typename T>
bool AdlSerialize(const T& obj, FILE* file) {
    return Serialize(obj, file);
}

template <typename T>
bool AdlDeserialize(T& obj, FILE* file) {
    return Deserialize(obj, file);
}
}  // namespace detail

/**
 * @brief Сериализатор для типов с пользовательской сериализацией
 *
 * Использует предоставленные пользовательские функции Serialize и Deserialize.
 * Эти функции должны быть доступны через ADL (аргументно-зависимый поиск) и
 * обычно объявляются в том же пространстве имен, что и тип данных.
 *
 * Функции должны иметь сигнатуры:
 * - bool Serialize(const T& obj, FILE* file);
 * - bool Deserialize(T& obj, FILE* file);
 */
template <CustomSerializable T>
class CustomFunctionSerializer : public Serializer<T> {
   public:
    bool Serialize(const T& obj, FILE* file) override {
        bool result = detail::AdlSerialize(obj, file);
        return result && !ferror(file);
    }

    bool Deserialize(T& obj, FILE* file) override {
        bool result = detail::AdlDeserialize(obj, file);
        return result && !ferror(file) && !feof(file);
    }
};

/**
 * @brief Сериализатор для типов с методами сериализации
 *
 * Использует методы Serialize и Deserialize класса
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
 * @brief Фабрика сериализаторов
 *
 * Создает соответствующий сериализатор в зависимости от типа данных
 */
template <typename T>
std::unique_ptr<Serializer<T>> create_serializer() {
    if constexpr (PodSerializable<T>) {
        return std::make_unique<PodSerializer<T>>();
    } else if constexpr (CustomSerializable<T>) {
        return std::make_unique<CustomFunctionSerializer<T>>();
    } else if constexpr (MethodSerializable<T>) {
        return std::make_unique<MethodSerializer<T>>();
    } else {
        static_assert(
            FileSerializable<T>, "Type must be serializable to be used with this library");
        return nullptr;
    }
}

template <>
class Serializer<std::string> {
   public:
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
    bool Serialize(const std::vector<T>& obj, FILE* file) {
        uint64_t size = obj.size();
        if (fwrite(&size, sizeof(uint64_t), 1, file) != 1) {
            return false;
        }

        auto item_serializer = create_serializer<T>();
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
        auto item_serializer = create_serializer<T>();
        for (auto& item : obj) {
            if (!item_serializer->Deserialize(item, file)) {
                return false;
            }
        }
        return true;
    }
};

}  // namespace external_sort
