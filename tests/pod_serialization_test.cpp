/**
 * @file pod_serialization_test.cpp
 * @brief Тесты для POD (Plain Old Data) сериализации
 * @author External Sort Library
 * @version 1.0
 */

#include "serializers.hpp"
#include "type_concepts.hpp"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <gtest/gtest.h>
#include <iostream>
#include <vector>

using namespace external_sort;

/**
 * @brief Фикстура для тестов POD сериализации
 */
class PodSerializationTest : public ::testing::Test {
   protected:
    void SetUp() override {
        test_file = "pod_test.bin";
        if (std::filesystem::exists(test_file)) {
            std::filesystem::remove(test_file);
        }
    }

    void TearDown() override {
        if (std::filesystem::exists(test_file)) {
            std::filesystem::remove(test_file);
        }
    }

    std::string test_file;

    template <typename T>
    void TestRoundTrip(const T& original) {
        auto serializer = create_serializer<T>();

        FILE* file = fopen(test_file.c_str(), "wb");
        ASSERT_NE(file, nullptr);
        EXPECT_TRUE(serializer->Serialize(original, file));
        fclose(file);

        T restored{};
        file = fopen(test_file.c_str(), "rb");
        ASSERT_NE(file, nullptr);
        EXPECT_TRUE(serializer->Deserialize(restored, file));
        fclose(file);

        EXPECT_EQ(original, restored);
    }
};

/**
 * @brief Тест базовых целочисленных типов
 */
TEST_F(PodSerializationTest, BasicIntegerTypes) {
    static_assert(PodSerializable<int8_t>);
    TestRoundTrip<int8_t>(-128);
    TestRoundTrip<int8_t>(127);
    TestRoundTrip<int8_t>(0);

    static_assert(PodSerializable<uint8_t>);
    TestRoundTrip<uint8_t>(0);
    TestRoundTrip<uint8_t>(255);

    static_assert(PodSerializable<int16_t>);
    TestRoundTrip<int16_t>(-32'768);
    TestRoundTrip<int16_t>(32'767);

    static_assert(PodSerializable<uint16_t>);
    TestRoundTrip<uint16_t>(0);
    TestRoundTrip<uint16_t>(65'535);

    static_assert(PodSerializable<int32_t>);
    TestRoundTrip<int32_t>(-2'147'483'648);
    TestRoundTrip<int32_t>(2'147'483'647);

    static_assert(PodSerializable<uint32_t>);
    TestRoundTrip<uint32_t>(0);
    TestRoundTrip<uint32_t>(4'294'967'295U);

    static_assert(PodSerializable<int64_t>);
    TestRoundTrip<int64_t>(-9'223'372'036'854'775'807LL - 1);
    TestRoundTrip<int64_t>(9'223'372'036'854'775'807LL);

    static_assert(PodSerializable<uint64_t>);
    TestRoundTrip<uint64_t>(0);
    TestRoundTrip<uint64_t>(18'446'744'073'709'551'615ULL);
}

/**
 * @brief Тест типов с плавающей точкой
 */
TEST_F(PodSerializationTest, FloatingPointTypes) {
    static_assert(PodSerializable<float>);
    TestRoundTrip<float>(0.0f);
    TestRoundTrip<float>(3.14159f);
    TestRoundTrip<float>(-2.71828f);
    TestRoundTrip<float>(1e6f);
    TestRoundTrip<float>(1e-6f);

    static_assert(PodSerializable<double>);
    TestRoundTrip<double>(0.0);
    TestRoundTrip<double>(3.141592653589793);
    TestRoundTrip<double>(-2.718281828459045);
    TestRoundTrip<double>(1e15);
    TestRoundTrip<double>(1e-15);
}

/**
 * @brief Тест символьных типов
 */
TEST_F(PodSerializationTest, CharacterTypes) {
    static_assert(PodSerializable<char>);
    TestRoundTrip<char>('A');
    TestRoundTrip<char>('z');
    TestRoundTrip<char>('\0');
    TestRoundTrip<char>('\n');

    static_assert(PodSerializable<unsigned char>);
    TestRoundTrip<unsigned char>(0);
    TestRoundTrip<unsigned char>(255);
}

/**
 * @brief Тест логического типа
 */
TEST_F(PodSerializationTest, BooleanType) {
    static_assert(PodSerializable<bool>);
    TestRoundTrip<bool>(true);
    TestRoundTrip<bool>(false);
}

/**
 * @brief Тест пользовательских POD структур
 */
TEST_F(PodSerializationTest, CustomPodStructures) {
    struct Point2D {
        float x, y;

        bool operator==(const Point2D& other) const {
            return x == other.x && y == other.y;
        }
    };

    static_assert(PodSerializable<Point2D>);
    TestRoundTrip<Point2D>({3.14f, 2.71f});
    TestRoundTrip<Point2D>({0.0f, 0.0f});
    TestRoundTrip<Point2D>({-1.0f, 1.0f});

    struct Vector3D {
        double x, y, z;
        int id;

        bool operator==(const Vector3D& other) const {
            return x == other.x && y == other.y && z == other.z && id == other.id;
        }
    };

    static_assert(PodSerializable<Vector3D>);
    TestRoundTrip<Vector3D>({1.0, 2.0, 3.0, 42});
    TestRoundTrip<Vector3D>({0.0, 0.0, 0.0, 0});
    TestRoundTrip<Vector3D>({-1.5, 2.5, -3.5, -100});

    struct FixedArray {
        int data[4];

        bool operator==(const FixedArray& other) const {
            return std::memcmp(data, other.data, sizeof(data)) == 0;
        }
    };

    static_assert(PodSerializable<FixedArray>);
    TestRoundTrip<FixedArray>({{1, 2, 3, 4}});
    TestRoundTrip<FixedArray>({{0, 0, 0, 0}});
    TestRoundTrip<FixedArray>({{-1, -2, -3, -4}});
}

/**
 * @brief Тест массовой сериализации
 */
TEST_F(PodSerializationTest, BulkSerialization) {
    std::vector<uint64_t> test_data;
    for (uint64_t i = 0; i < 1000; ++i) {
        test_data.push_back(i * i);
    }

    auto serializer = create_serializer<uint64_t>();

    FILE* file = fopen(test_file.c_str(), "wb");
    ASSERT_NE(file, nullptr);

    for (const auto& value : test_data) {
        EXPECT_TRUE(serializer->Serialize(value, file));
    }
    fclose(file);

    std::vector<uint64_t> restored_data;
    restored_data.reserve(test_data.size());

    file = fopen(test_file.c_str(), "rb");
    ASSERT_NE(file, nullptr);

    for (uint64_t i = 0; i < test_data.size(); ++i) {
        uint64_t value;
        EXPECT_TRUE(serializer->Deserialize(value, file));
        restored_data.push_back(value);
    }
    fclose(file);

    EXPECT_EQ(test_data, restored_data);
}

/**
 * @brief Тест ошибок сериализации
 */
TEST_F(PodSerializationTest, SerializationErrors) {
    auto serializer = create_serializer<int>();
    int test_value = 42;
    int restored_value = 0;

    FILE* file = fopen(test_file.c_str(), "wb");
    ASSERT_NE(file, nullptr);
    fclose(file);

    EXPECT_FALSE(serializer->Serialize(test_value, file));

    EXPECT_FALSE(serializer->Deserialize(restored_value, file));

    file = fopen(test_file.c_str(), "wb");
    ASSERT_NE(file, nullptr);
    fclose(file);

    file = fopen(test_file.c_str(), "rb");
    ASSERT_NE(file, nullptr);
    EXPECT_FALSE(serializer->Deserialize(restored_value, file));
    fclose(file);
}
