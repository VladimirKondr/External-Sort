/**
 * @file external_adl_serialization_test.cpp
 * @brief Тесты для external/ADL сериализации (функции serialize/deserialize вне класса)
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
 * @brief Пространство имен для тестовых типов с ADL сериализацией
 */
namespace test_types {

/**
 * @brief Простая структура для тестирования ADL
 */
struct Person {
    std::string name;
    int age;
    double height;

    bool operator==(const Person& other) const {
        return name == other.name && age == other.age && height == other.height;
    }
};

/**
 * @brief Функции ADL сериализации для Person
 * Должны быть в том же пространстве имен для работы ADL
 */
bool serialize(const Person& person, FILE* file) {
    uint64_t name_size = person.name.size();
    if (fwrite(&name_size, sizeof(uint64_t), 1, file) != 1) {
        return false;
    }
    if (name_size > 0) {
        if (fwrite(person.name.data(), sizeof(char), name_size, file) != name_size) {
            return false;
        }
    }

    if (fwrite(&person.age, sizeof(int), 1, file) != 1) {
        return false;
    }

    if (fwrite(&person.height, sizeof(double), 1, file) != 1) {
        return false;
    }

    return true;
}

bool deserialize(Person& person, FILE* file) {
    uint64_t name_size;
    if (fread(&name_size, sizeof(uint64_t), 1, file) != 1) {
        return false;
    }
    person.name.resize(name_size);
    if (name_size > 0) {
        if (fread(&person.name[0], sizeof(char), name_size, file) != name_size) {
            return false;
        }
    }

    if (fread(&person.age, sizeof(int), 1, file) != 1) {
        return false;
    }

    if (fread(&person.height, sizeof(double), 1, file) != 1) {
        return false;
    }

    return true;
}

/**
 * @brief Сложная структура для тестирования вложенной ADL сериализации
 */
struct Company {
    std::string name;
    std::vector<Person> employees;
    double revenue;

    bool operator==(const Company& other) const {
        return name == other.name && employees == other.employees && revenue == other.revenue;
    }
};

bool serialize(const Company& company, FILE* file) {
    uint64_t name_size = company.name.size();
    if (fwrite(&name_size, sizeof(uint64_t), 1, file) != 1) {
        return false;
    }
    if (name_size > 0) {
        if (fwrite(company.name.data(), sizeof(char), name_size, file) != name_size) {
            return false;
        }
    }

    uint64_t employees_count = company.employees.size();
    if (fwrite(&employees_count, sizeof(uint64_t), 1, file) != 1) {
        return false;
    }
    for (const auto& employee : company.employees) {
        if (!serialize(employee, file)) {
            return false;
        }
    }

    if (fwrite(&company.revenue, sizeof(double), 1, file) != 1) {
        return false;
    }

    return true;
}

bool deserialize(Company& company, FILE* file) {
    uint64_t name_size;
    if (fread(&name_size, sizeof(uint64_t), 1, file) != 1) {
        return false;
    }
    company.name.resize(name_size);
    if (name_size > 0) {
        if (fread(&company.name[0], sizeof(char), name_size, file) != name_size) {
            return false;
        }
    }

    uint64_t employees_count;
    if (fread(&employees_count, sizeof(uint64_t), 1, file) != 1) {
        return false;
    }
    company.employees.resize(employees_count);
    for (auto& employee : company.employees) {
        if (!deserialize(employee, file)) {
            return false;
        }
    }

    if (fread(&company.revenue, sizeof(double), 1, file) != 1) {
        return false;
    }

    return true;
}

/**
 * @brief Простая структура с числовыми данными
 */
struct Point3D {
    float x, y, z;

    Point3D() = default;

    Point3D(float x_val, float y_val, float z_val) : x(x_val), y(y_val), z(z_val) {
    }

    virtual ~Point3D() = default;

    bool operator==(const Point3D& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
};

bool serialize(const Point3D& point, FILE* file) {
    if (fwrite(&point.x, sizeof(float), 1, file) != 1) {
        return false;
    }
    if (fwrite(&point.y, sizeof(float), 1, file) != 1) {
        return false;
    }
    return fwrite(&point.z, sizeof(float), 1, file) == 1;
}

bool deserialize(Point3D& point, FILE* file) {
    if (fread(&point.x, sizeof(float), 1, file) != 1) {
        return false;
    }
    if (fread(&point.y, sizeof(float), 1, file) != 1) {
        return false;
    }
    return fread(&point.z, sizeof(float), 1, file) == 1;
}

/**
 * @brief Структура с массивом фиксированного размера
 */
struct Matrix2x2 {
    float data[2][2];

    Matrix2x2() = default;

    Matrix2x2(float d00, float d01, float d10, float d11) {
        data[0][0] = d00;
        data[0][1] = d01;
        data[1][0] = d10;
        data[1][1] = d11;
    }

    virtual ~Matrix2x2() = default;

    bool operator==(const Matrix2x2& other) const {
        for (int i = 0; i < 2; ++i) {
            for (int j = 0; j < 2; ++j) {
                if (data[i][j] != other.data[i][j]) {
                    return false;
                }
            }
        }
        return true;
    }
};

bool serialize(const Matrix2x2& matrix, FILE* file) {
    return fwrite(matrix.data, sizeof(float), 4, file) == 4;
}

bool deserialize(Matrix2x2& matrix, FILE* file) {
    return fread(matrix.data, sizeof(float), 4, file) == 4;
}

}  // namespace test_types

/**
 * @brief Фикстура для тестов external/ADL сериализации
 */
class ExternalAdlSerializationTest : public ::testing::Test {
   protected:
    void SetUp() override {
        test_file = "external_test.bin";
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
        EXPECT_TRUE(serializer->serialize(original, file));
        fclose(file);

        T restored{};
        file = fopen(test_file.c_str(), "rb");
        ASSERT_NE(file, nullptr);
        EXPECT_TRUE(serializer->deserialize(restored, file));
        fclose(file);

        EXPECT_EQ(original, restored);
    }
};

/**
 * @brief Тест ADL сериализации для Person
 */
TEST_F(ExternalAdlSerializationTest, PersonSerialization) {
    using test_types::Person;

    static_assert(CustomSerializable<Person>);
    static_assert(!PodSerializable<Person>);
    static_assert(!MethodSerializable<Person>);

    TestRoundTrip<Person>({"John Doe", 30, 175.5});
    TestRoundTrip<Person>({"", 0, 0.0});
    TestRoundTrip<Person>({"Alice", 25, 160.0});

    TestRoundTrip<Person>({"Very Long Name With Many Characters And Spaces", 100, 200.5});
    TestRoundTrip<Person>({"Иван Иванов", 45, 180.0});
}

/**
 * @brief Тест ADL сериализации для Point3D
 */
TEST_F(ExternalAdlSerializationTest, Point3DSerialization) {
    using test_types::Point3D;

    static_assert(CustomSerializable<Point3D>);
    static_assert(!PodSerializable<Point3D>);

    TestRoundTrip<Point3D>(Point3D(0.0f, 0.0f, 0.0f));
    TestRoundTrip<Point3D>(Point3D(1.0f, 2.0f, 3.0f));
    TestRoundTrip<Point3D>(Point3D(-5.5f, 10.25f, -15.75f));
    TestRoundTrip<Point3D>(Point3D(1e6f, 1e-6f, 3.14159f));
}

/**
 * @brief Тест ADL сериализации для Matrix2x2
 */
TEST_F(ExternalAdlSerializationTest, Matrix2x2Serialization) {
    using test_types::Matrix2x2;

    static_assert(CustomSerializable<Matrix2x2>);

    TestRoundTrip<Matrix2x2>(Matrix2x2(1.0f, 0.0f, 0.0f, 1.0f));

    TestRoundTrip<Matrix2x2>(Matrix2x2(0.0f, 0.0f, 0.0f, 0.0f));

    TestRoundTrip<Matrix2x2>(Matrix2x2(1.5f, 2.5f, 3.5f, 4.5f));
}

/**
 * @brief Тест вложенной ADL сериализации для Company
 */
TEST_F(ExternalAdlSerializationTest, CompanySerialization) {
    using test_types::Company;
    using test_types::Person;

    static_assert(CustomSerializable<Company>);

    TestRoundTrip<Company>({"Empty Corp", {}, 0.0});

    TestRoundTrip<Company>({"Small Business", {{"Owner", 35, 170.0}}, 50000.0});

    std::vector<Person> employees = {
      {"Alice Johnson", 28, 165.0}, {"Bob Smith", 32, 180.0}, {"Carol Brown", 45, 170.0}};
    TestRoundTrip<Company>({"Tech Corp", employees, 1000000.0});

    std::vector<Person> many_employees;
    for (int i = 0; i < 50; ++i) {
        many_employees.push_back({"Employee " + std::to_string(i), 25 + i % 40, 160.0 + i % 50});
    }
    TestRoundTrip<Company>({"Big Corporation", many_employees, 50000000.0});
}

/**
 * @brief Тест обработки ошибок в ADL сериализации
 */
TEST_F(ExternalAdlSerializationTest, SerializationErrors) {
    using test_types::Person;

    Person person{"Test Person", 30, 175.0};
    auto serializer = create_serializer<Person>();

    FILE* file = fopen(test_file.c_str(), "wb");
    ASSERT_NE(file, nullptr);
    fclose(file);

    EXPECT_FALSE(serializer->serialize(person, file));

    Person restored_person;
    EXPECT_FALSE(serializer->deserialize(restored_person, file));

    file = fopen(test_file.c_str(), "wb");
    ASSERT_NE(file, nullptr);
    fclose(file);

    file = fopen(test_file.c_str(), "rb");
    ASSERT_NE(file, nullptr);
    EXPECT_FALSE(serializer->deserialize(restored_person, file));
    fclose(file);
}

/**
 * @brief Тест массовой ADL сериализации
 */
TEST_F(ExternalAdlSerializationTest, BulkSerialization) {
    using test_types::Point3D;

    const uint64_t test_size = 1000;
    std::vector<Point3D> test_data;
    test_data.reserve(test_size);

    for (uint64_t i = 0; i < test_size; ++i) {
        test_data.push_back(
            Point3D(static_cast<float>(i), static_cast<float>(i * 2), static_cast<float>(i * 3)));
    }

    auto serializer = create_serializer<Point3D>();

    FILE* file = fopen(test_file.c_str(), "wb");
    ASSERT_NE(file, nullptr);

    for (const auto& point : test_data) {
        EXPECT_TRUE(serializer->serialize(point, file));
    }
    fclose(file);

    std::vector<Point3D> restored_data;
    restored_data.reserve(test_size);

    file = fopen(test_file.c_str(), "rb");
    ASSERT_NE(file, nullptr);

    for (uint64_t i = 0; i < test_size; ++i) {
        Point3D point;
        EXPECT_TRUE(serializer->deserialize(point, file));
        restored_data.push_back(point);
    }
    fclose(file);

    EXPECT_EQ(test_data, restored_data);
}

/**
 * @brief Тест приоритета сериализаторов
 */
TEST_F(ExternalAdlSerializationTest, SerializerPriorityTest) {
    static_assert(PodSerializable<int>);
    static_assert(!CustomSerializable<int>);
    static_assert(!MethodSerializable<int>);

    using test_types::Person;
    static_assert(!PodSerializable<Person>);
    static_assert(CustomSerializable<Person>);
    static_assert(!MethodSerializable<Person>);

    auto int_serializer = create_serializer<int>();
    auto person_serializer = create_serializer<Person>();

    ASSERT_NE(int_serializer, nullptr);
    ASSERT_NE(person_serializer, nullptr);

    TestRoundTrip<int>(42);
    TestRoundTrip<Person>({"Test", 25, 170.0});
}