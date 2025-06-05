/**
 * @file method_serialization_test.cpp
 * @brief Тесты для method-based сериализации (классы с методами Serialize/Deserialize)
 * @author External Sort Library
 * @version 1.0
 */

#include "serializers.hpp"
#include "type_concepts.hpp"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <vector>

using namespace external_sort;

/**
 * @brief Фикстура для тестов method сериализации
 */
class MethodSerializationTest : public ::testing::Test {
   protected:
    void SetUp() override {
        test_file = "method_test.bin";
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
 * @brief Простой класс с методами сериализации
 */
class SimplePoint {
   public:
    int x, y;

    SimplePoint() : x(0), y(0) {
    }

    SimplePoint(int x_val, int y_val) : x(x_val), y(y_val) {
    }

    virtual ~SimplePoint() = default;

    bool Serialize(FILE* file) const {
        if (fwrite(&x, sizeof(int), 1, file) != 1) {
            return false;
        }
        return fwrite(&y, sizeof(int), 1, file) == 1;
    }

    bool Deserialize(FILE* file) {
        if (fread(&x, sizeof(int), 1, file) != 1) {
            return false;
        }
        return fread(&y, sizeof(int), 1, file) == 1;
    }

    bool operator==(const SimplePoint& other) const {
        return x == other.x && y == other.y;
    }
};

/**
 * @brief Класс с переменными данными
 */
class VariableData {
   public:
    std::string name;
    std::vector<int> values;

    VariableData() = default;

    VariableData(const std::string& n, const std::vector<int>& v) : name(n), values(v) {
    }

    bool Serialize(FILE* file) const {
        uint64_t name_size = name.size();
        if (fwrite(&name_size, sizeof(uint64_t), 1, file) != 1) {
            return false;
        }
        if (fwrite(name.data(), sizeof(char), name_size, file) != name_size) {
            return false;
        }

        uint64_t values_size = values.size();
        if (fwrite(&values_size, sizeof(uint64_t), 1, file) != 1) {
            return false;
        }
        if (values_size > 0) {
            if (fwrite(values.data(), sizeof(int), values_size, file) != values_size) {
                return false;
            }
        }

        return true;
    }

    bool Deserialize(FILE* file) {
        uint64_t name_size;
        if (fread(&name_size, sizeof(uint64_t), 1, file) != 1) {
            return false;
        }
        name.resize(name_size);
        if (name_size > 0) {
            if (fread(&name[0], sizeof(char), name_size, file) != name_size) {
                return false;
            }
        }

        uint64_t values_size;
        if (fread(&values_size, sizeof(uint64_t), 1, file) != 1) {
            return false;
        }
        values.resize(values_size);
        if (values_size > 0) {
            if (fread(values.data(), sizeof(int), values_size, file) != values_size) {
                return false;
            }
        }

        return true;
    }

    bool operator==(const VariableData& other) const {
        return name == other.name && values == other.values;
    }
};

/**
 * @brief Сложный класс с вложенными данными
 */
class ComplexObject {
   public:
    double coefficient;
    SimplePoint position;
    std::vector<SimplePoint> points;

    ComplexObject() : coefficient(0.0) {
    }

    ComplexObject(double coeff, const SimplePoint& pos, const std::vector<SimplePoint>& pts)
        : coefficient(coeff), position(pos), points(pts) {
    }

    bool Serialize(FILE* file) const {
        if (fwrite(&coefficient, sizeof(double), 1, file) != 1) {
            return false;
        }

        if (!position.Serialize(file)) {
            return false;
        }

        uint64_t points_size = points.size();
        if (fwrite(&points_size, sizeof(uint64_t), 1, file) != 1) {
            return false;
        }
        for (const auto& point : points) {
            if (!point.Serialize(file)) {
                return false;
            }
        }

        return true;
    }

    bool Deserialize(FILE* file) {
        if (fread(&coefficient, sizeof(double), 1, file) != 1) {
            return false;
        }

        if (!position.Deserialize(file)) {
            return false;
        }

        uint64_t points_size;
        if (fread(&points_size, sizeof(uint64_t), 1, file) != 1) {
            return false;
        }
        points.resize(points_size);
        for (auto& point : points) {
            if (!point.Deserialize(file)) {
                return false;
            }
        }

        return true;
    }

    bool operator==(const ComplexObject& other) const {
        return coefficient == other.coefficient && position == other.position &&
               points == other.points;
    }
};

/**
 * @brief Тест простого класса с методами сериализации
 */
TEST_F(MethodSerializationTest, SimplePointSerialization) {
    static_assert(MethodSerializable<SimplePoint>);
    static_assert(!PodSerializable<SimplePoint>);

    TestRoundTrip<SimplePoint>({0, 0});
    TestRoundTrip<SimplePoint>({10, 20});
    TestRoundTrip<SimplePoint>({-5, -10});
    TestRoundTrip<SimplePoint>({1'000'000, -1'000'000});
}

/**
 * @brief Тест класса с переменными данными
 */
TEST_F(MethodSerializationTest, VariableDataSerialization) {
    static_assert(MethodSerializable<VariableData>);

    TestRoundTrip<VariableData>({"", {}});

    TestRoundTrip<VariableData>({"test", {1, 2, 3}});

    TestRoundTrip<VariableData>({"complex_name_with_underscores", {-1, 0, 1, 100, -100, 42}});

    std::string long_name(1000, 'x');
    std::vector<int> many_values;
    for (int i = 0; i < 1000; ++i) {
        many_values.push_back(i * i);
    }
    TestRoundTrip<VariableData>({long_name, many_values});
}

/**
 * @brief Тест сложного объекта с вложенными данными
 */
TEST_F(MethodSerializationTest, ComplexObjectSerialization) {
    static_assert(MethodSerializable<ComplexObject>);

    TestRoundTrip<ComplexObject>({3.14, {10, 20}, {}});

    std::vector<SimplePoint> points = {{1, 2}, {3, 4}, {5, 6}};
    TestRoundTrip<ComplexObject>({2.71, {0, 0}, points});

    std::vector<SimplePoint> many_points;
    for (int i = 0; i < 100; ++i) {
        many_points.push_back({i, i * 2});
    }
    TestRoundTrip<ComplexObject>({1.41421, {-50, 50}, many_points});
}

/**
 * @brief Тест обработки ошибок сериализации
 */
TEST_F(MethodSerializationTest, SerializationErrors) {
    SimplePoint point(42, 24);
    auto serializer = create_serializer<SimplePoint>();

    FILE* file = fopen(test_file.c_str(), "wb");
    ASSERT_NE(file, nullptr);
    fclose(file);

    EXPECT_FALSE(serializer->Serialize(point, file));

    SimplePoint restored_point;
    EXPECT_FALSE(serializer->Deserialize(restored_point, file));

    file = fopen(test_file.c_str(), "wb");
    ASSERT_NE(file, nullptr);
    fclose(file);

    file = fopen(test_file.c_str(), "rb");
    ASSERT_NE(file, nullptr);
    EXPECT_FALSE(serializer->Deserialize(restored_point, file));
    fclose(file);

    file = fopen(test_file.c_str(), "wb");
    ASSERT_NE(file, nullptr);
    int partial_data = 42;
    fwrite(&partial_data, sizeof(int), 1, file);
    fclose(file);

    file = fopen(test_file.c_str(), "rb");
    ASSERT_NE(file, nullptr);
    EXPECT_FALSE(serializer->Deserialize(restored_point, file));
    fclose(file);
}

/**
 * @brief Тест массовой сериализации объектов
 */
TEST_F(MethodSerializationTest, BulkSerialization) {
    const uint64_t test_size = 1000;
    std::vector<SimplePoint> test_data;
    test_data.reserve(test_size);

    for (uint64_t i = 0; i < test_size; ++i) {
        test_data.push_back({static_cast<int>(i), static_cast<int>(i * 2)});
    }

    auto serializer = create_serializer<SimplePoint>();

    FILE* file = fopen(test_file.c_str(), "wb");
    ASSERT_NE(file, nullptr);

    for (const auto& point : test_data) {
        EXPECT_TRUE(serializer->Serialize(point, file));
    }
    fclose(file);

    std::vector<SimplePoint> restored_data;
    restored_data.reserve(test_size);

    file = fopen(test_file.c_str(), "rb");
    ASSERT_NE(file, nullptr);

    for (uint64_t i = 0; i < test_size; ++i) {
        SimplePoint point;
        EXPECT_TRUE(serializer->Deserialize(point, file));
        restored_data.push_back(point);
    }
    fclose(file);

    EXPECT_EQ(test_data, restored_data);
}
