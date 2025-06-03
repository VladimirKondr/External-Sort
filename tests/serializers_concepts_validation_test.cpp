/**
 * @file serializers_concepts_validation_test.cpp
 * @brief Тесты для валидации концептов сериализации и функциональности сериализаторов
 * @author External Sort Library
 * @version 1.0
 */

#include "serializers.hpp"
#include "type_concepts.hpp"

#include <cstdio>
#include <filesystem>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

using namespace external_sort;

/**
 * @brief Пространство имен для тестовых типов
 */
namespace concept_test_types {

struct SimplePod {
    int x, y;

    bool operator==(const SimplePod& other) const {
        return x == other.x && y == other.y;
    }
};

class NonSerializable {
    std::unique_ptr<int> ptr;

   public:
    NonSerializable() : ptr(std::make_unique<int>(42)) {
    }
};

class WithMethods {
   public:
    int value;

    WithMethods() : value(0) {
    }

    WithMethods(int v) : value(v) {
    }

    virtual ~WithMethods() = default;

    bool serialize(FILE* file) const {
        return fwrite(&value, sizeof(int), 1, file) == 1;
    }

    bool deserialize(FILE* file) {
        return fread(&value, sizeof(int), 1, file) == 1;
    }

    bool operator==(const WithMethods& other) const {
        return value == other.value;
    }
};

struct OnlyAdlFunctions {
    double data;

    bool operator==(const OnlyAdlFunctions& other) const {
        return data == other.data;
    }
};

bool serialize(const OnlyAdlFunctions& obj, FILE* file) {
    return fwrite(&obj.data, sizeof(double), 1, file) == 1;
}

bool deserialize(OnlyAdlFunctions& obj, FILE* file) {
    return fread(&obj.data, sizeof(double), 1, file) == 1;
}

struct WithAdlFunctions {
    double data;

    bool operator==(const WithAdlFunctions& other) const {
        return data == other.data;
    }
};

bool serialize(const WithAdlFunctions& obj, FILE* file) {
    return fwrite(&obj.data, sizeof(double), 1, file) == 1;
}

bool deserialize(WithAdlFunctions& obj, FILE* file) {
    return fread(&obj.data, sizeof(double), 1, file) == 1;
}

class WithWrongMethods {
   public:
    int value;

    void serialize(FILE* file) const {
        fwrite(&value, sizeof(int), 1, file);
    }

    void deserialize(FILE* file) {
        fread(&value, sizeof(int), 1, file);
    }
};

class WithWrongParameters {
   public:
    int value;

    bool serialize(int file) const {
        return true;
    }

    bool deserialize(int file) {
        return true;
    }
};

}  // namespace concept_test_types

/**
 * @brief Тест валидации концептов типов
 */
class ConceptValidationTest : public ::testing::Test {
   protected:
    void SetUp() override {
        test_file = "concept_test.bin";
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
};

/**
 * @brief Тест концепта PodSerializable
 */
TEST_F(ConceptValidationTest, PodSerializableConcept) {
    static_assert(PodSerializable<int>);
    static_assert(PodSerializable<float>);
    static_assert(PodSerializable<double>);
    static_assert(PodSerializable<char>);
    static_assert(PodSerializable<bool>);
    static_assert(PodSerializable<uint64_t>);
    static_assert(PodSerializable<int32_t>);

    static_assert(PodSerializable<concept_test_types::SimplePod>);

    static_assert(!PodSerializable<std::string>);
    static_assert(!PodSerializable<std::vector<int>>);
    static_assert(!PodSerializable<concept_test_types::WithMethods>);
    static_assert(!PodSerializable<concept_test_types::NonSerializable>);
}

/**
 * @brief Тест концепта MethodSerializable
 */
TEST_F(ConceptValidationTest, MethodSerializableConcept) {
    static_assert(MethodSerializable<concept_test_types::WithMethods>);

    static_assert(!MethodSerializable<int>);
    static_assert(!MethodSerializable<std::string>);
    static_assert(!MethodSerializable<concept_test_types::SimplePod>);
    static_assert(!MethodSerializable<concept_test_types::NonSerializable>);

    static_assert(!MethodSerializable<concept_test_types::WithWrongMethods>);
    static_assert(!MethodSerializable<concept_test_types::WithWrongParameters>);
}

/**
 * @brief Тест концепта CustomSerializable
 */
TEST_F(ConceptValidationTest, CustomSerializableConcept) {
    static_assert(CustomSerializable<concept_test_types::OnlyAdlFunctions>);
    static_assert(CustomSerializable<concept_test_types::WithAdlFunctions>);

    static_assert(!CustomSerializable<int>);
    static_assert(!CustomSerializable<concept_test_types::SimplePod>);
    static_assert(!CustomSerializable<concept_test_types::WithMethods>);
    static_assert(!CustomSerializable<concept_test_types::NonSerializable>);
}

/**
 * @brief Тест концепта SpecializedSerializable
 */
TEST_F(ConceptValidationTest, SpecializedSerializableConcept) {
    static_assert(SpecializedSerializable<std::string>);

    static_assert(SpecializedSerializable<std::vector<int>>);
    static_assert(SpecializedSerializable<std::vector<double>>);
    static_assert(SpecializedSerializable<std::vector<std::string>>);

    static_assert(!SpecializedSerializable<int>);
    static_assert(!SpecializedSerializable<concept_test_types::SimplePod>);
    static_assert(!SpecializedSerializable<concept_test_types::WithMethods>);
}

/**
 * @brief Тест концепта FileSerializable
 */
TEST_F(ConceptValidationTest, FileSerializableConcept) {
    static_assert(FileSerializable<int>);
    static_assert(FileSerializable<concept_test_types::SimplePod>);
    static_assert(FileSerializable<concept_test_types::WithMethods>);
    static_assert(FileSerializable<concept_test_types::OnlyAdlFunctions>);
    static_assert(FileSerializable<concept_test_types::WithAdlFunctions>);
    static_assert(FileSerializable<std::string>);
    static_assert(FileSerializable<std::vector<int>>);

    static_assert(!FileSerializable<concept_test_types::NonSerializable>);
}

/**
 * @brief Тест приоритета выбора сериализаторов
 */
TEST_F(ConceptValidationTest, SerializerPriority) {
    {
        auto serializer = create_serializer<int>();
        ASSERT_NE(serializer, nullptr);

        auto* pod_serializer = dynamic_cast<PodSerializer<int>*>(serializer.get());
        EXPECT_NE(pod_serializer, nullptr);
    }

    {
        concept_test_types::OnlyAdlFunctions original{42.42};
        auto serializer = create_serializer<concept_test_types::OnlyAdlFunctions>();
        ASSERT_NE(serializer, nullptr);
        FILE* file1 = fopen(test_file.c_str(), "wb");
        ASSERT_NE(file1, nullptr);
        EXPECT_TRUE(serializer->serialize(original, file1));
        fclose(file1);
        concept_test_types::OnlyAdlFunctions restored1{};
        FILE* file2 = fopen(test_file.c_str(), "rb");
        ASSERT_NE(file2, nullptr);
        EXPECT_TRUE(serializer->deserialize(restored1, file2));
        fclose(file2);
        EXPECT_EQ(original, restored1);
    }
    {
        concept_test_types::WithAdlFunctions original{99.99};
        auto serializer = create_serializer<concept_test_types::WithAdlFunctions>();
        ASSERT_NE(serializer, nullptr);
        FILE* file3 = fopen(test_file.c_str(), "wb");
        ASSERT_NE(file3, nullptr);
        EXPECT_TRUE(serializer->serialize(original, file3));
        fclose(file3);
        concept_test_types::WithAdlFunctions restored2{};
        FILE* file4 = fopen(test_file.c_str(), "rb");
        ASSERT_NE(file4, nullptr);
        EXPECT_TRUE(serializer->deserialize(restored2, file4));
        fclose(file4);
        EXPECT_EQ(original, restored2);
    }
}

/**
 * @brief Тест функциональности всех типов сериализаторов
 */
TEST_F(ConceptValidationTest, SerializerFunctionality) {
    {
        concept_test_types::SimplePod original{42, 24};
        auto serializer = create_serializer<concept_test_types::SimplePod>();

        FILE* file = fopen(test_file.c_str(), "wb");
        ASSERT_NE(file, nullptr);
        EXPECT_TRUE(serializer->serialize(original, file));
        fclose(file);

        concept_test_types::SimplePod restored{};
        file = fopen(test_file.c_str(), "rb");
        ASSERT_NE(file, nullptr);
        EXPECT_TRUE(serializer->deserialize(restored, file));
        fclose(file);

        EXPECT_EQ(original, restored);
    }

    {
        concept_test_types::WithMethods original{100};
        auto serializer = create_serializer<concept_test_types::WithMethods>();

        FILE* file = fopen(test_file.c_str(), "wb");
        ASSERT_NE(file, nullptr);
        EXPECT_TRUE(serializer->serialize(original, file));
        fclose(file);

        concept_test_types::WithMethods restored{};
        file = fopen(test_file.c_str(), "rb");
        ASSERT_NE(file, nullptr);
        EXPECT_TRUE(serializer->deserialize(restored, file));
        fclose(file);

        EXPECT_EQ(original, restored);
    }

    {
        concept_test_types::OnlyAdlFunctions original{3.14159};
        auto serializer = create_serializer<concept_test_types::OnlyAdlFunctions>();

        FILE* file = fopen(test_file.c_str(), "wb");
        ASSERT_NE(file, nullptr);
        EXPECT_TRUE(serializer->serialize(original, file));
        fclose(file);

        concept_test_types::OnlyAdlFunctions restored{};
        file = fopen(test_file.c_str(), "rb");
        ASSERT_NE(file, nullptr);
        EXPECT_TRUE(serializer->deserialize(restored, file));
        fclose(file);

        EXPECT_EQ(original, restored);
    }
}

/**
 * @brief Тест специализаций для std::string и std::vector
 */
TEST_F(ConceptValidationTest, SpecializedSerializers) {
    {
        std::string original = "Hello, World! Тест unicode строки 🚀";

        Serializer<std::string> serializer;

        FILE* file = fopen(test_file.c_str(), "wb");
        ASSERT_NE(file, nullptr);
        EXPECT_TRUE(serializer.serialize(original, file));
        fclose(file);

        std::string restored;
        file = fopen(test_file.c_str(), "rb");
        ASSERT_NE(file, nullptr);
        EXPECT_TRUE(serializer.deserialize(restored, file));
        fclose(file);

        EXPECT_EQ(original, restored);
    }

    {
        std::vector<int> original = {1, 2, 3, 4, 5, -1, -2, -3};

        Serializer<std::vector<int>> serializer;

        FILE* file = fopen(test_file.c_str(), "wb");
        ASSERT_NE(file, nullptr);
        EXPECT_TRUE(serializer.serialize(original, file));
        fclose(file);

        std::vector<int> restored;
        file = fopen(test_file.c_str(), "rb");
        ASSERT_NE(file, nullptr);
        EXPECT_TRUE(serializer.deserialize(restored, file));
        fclose(file);

        EXPECT_EQ(original, restored);
    }

    {
        std::vector<std::vector<double>> original = {{1.1, 2.2, 3.3}, {}, {4.4, 5.5}, {6.6}};

        Serializer<std::vector<std::vector<double>>> serializer;

        FILE* file = fopen(test_file.c_str(), "wb");
        ASSERT_NE(file, nullptr);
        EXPECT_TRUE(serializer.serialize(original, file));
        fclose(file);

        std::vector<std::vector<double>> restored;
        file = fopen(test_file.c_str(), "rb");
        ASSERT_NE(file, nullptr);
        EXPECT_TRUE(serializer.deserialize(restored, file));
        fclose(file);

        EXPECT_EQ(original, restored);
    }
}

/**
 * @brief Тест ошибок компиляции для несериализуемых типов
 */
TEST_F(ConceptValidationTest, CompilationErrors) {
    static_assert(!FileSerializable<concept_test_types::NonSerializable>);
    static_assert(!FileSerializable<std::unique_ptr<int>>);

    SUCCEED();
}
