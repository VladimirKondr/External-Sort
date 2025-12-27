/**
 * @file serializing_test.cpp
 * @brief Tests for validating type concepts
 */

#include "type_concepts.hpp"

#include "serializers.hpp"

#include <cstdio>
#include <filesystem>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

#include "../../logging/include/Registry.hpp"

using serialization::CreateSerializer;
using serialization::CustomSerializable;
using serialization::FileSerializable;
using serialization::MethodSerializable;
using serialization::PodSerializable;

/**
 * @brief Namespace for test types
 */
namespace concept_test_types {

struct SimplePod {
    int x, y;

    bool operator==(const SimplePod& other) const {
        return x == other.x && y == other.y;
    }
};

class NonSerializable {
    std::unique_ptr<int> ptr_;

public:
    NonSerializable() : ptr_(std::make_unique<int>(42)) {
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

    bool Serialize(FILE* file) const {
        return fwrite(&value, sizeof(int), 1, file) == 1;
    }

    bool Deserialize(FILE* file) {
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

bool Serialize(const OnlyAdlFunctions& obj, FILE* file) {
    return fwrite(&obj.data, sizeof(double), 1, file) == 1;
}

bool Deserialize(OnlyAdlFunctions& obj, FILE* file) {
    return fread(&obj.data, sizeof(double), 1, file) == 1;
}

struct WithAdlFunctions {
    double data;

    bool operator==(const WithAdlFunctions& other) const {
        return data == other.data;
    }
};

bool Serialize(const WithAdlFunctions& obj, FILE* file) {
    return fwrite(&obj.data, sizeof(double), 1, file) == 1;
}

bool Deserialize(WithAdlFunctions& obj, FILE* file) {
    return fread(&obj.data, sizeof(double), 1, file) == 1;
}

/**
 * @brief Complex class with multiple member types
 */
class ComplexData {
public:
    int integer_value;
    double double_value;
    std::string text;
    WithMethods nested_object;

    ComplexData() : integer_value(0), double_value(0.0), nested_object(0) {
    }

    ComplexData(int i, double d, const std::string& s, int nested_val)
        : integer_value(i), double_value(d), text(s), nested_object(nested_val) {
    }

    bool Serialize(FILE* file) const {
        if (fwrite(&integer_value, sizeof(int), 1, file) != 1) {
            return false;
        }

        if (fwrite(&double_value, sizeof(double), 1, file) != 1) {
            return false;
        }

        auto string_serializer = serialization::CreateSerializer<std::string>();
        if (!string_serializer->Serialize(text, file)) {
            return false;
        }

        auto nested_object_serializer = serialization::CreateSerializer<WithMethods>();
        if (!nested_object_serializer->Serialize(nested_object, file)) {
            return false;
        }

        return true;
    }

    bool Deserialize(FILE* file) {
        if (fread(&integer_value, sizeof(int), 1, file) != 1) {
            return false;
        }

        if (fread(&double_value, sizeof(double), 1, file) != 1) {
            return false;
        }

        auto string_serializer = serialization::CreateSerializer<std::string>();
        if (!string_serializer->Deserialize(text, file)) {
            return false;
        }

        auto nested_object_serializer = serialization::CreateSerializer<WithMethods>();
        if (!nested_object_serializer->Deserialize(nested_object, file)) {
            return false;
        }

        return true;
    }

    bool operator==(const ComplexData& other) const {
        return integer_value == other.integer_value && double_value == other.double_value &&
               text == other.text && nested_object == other.nested_object;
    }
};

}  // namespace concept_test_types

/**
 * @brief Test fixture for type concept validation
 */
class ConceptValidationTest : public ::testing::Test {
protected:
    void SetUp() override {
        logging::SetDefaultLogger();

        test_file_ = "concept_test.bin";
        if (std::filesystem::exists(test_file_)) {
            std::filesystem::remove(test_file_);
        }
    }

    void TearDown() override {
        if (std::filesystem::exists(test_file_)) {
            std::filesystem::remove(test_file_);
        }
    }

    std::string test_file_;
};

/**
 * @brief Test for all serializer types functionality
 */
TEST_F(ConceptValidationTest, SerializerFunctionality) {
    {
        concept_test_types::SimplePod original{42, 24};
        auto serializer = CreateSerializer<concept_test_types::SimplePod>();

        FILE* file = fopen(test_file_.c_str(), "wb");
        ASSERT_NE(file, nullptr);
        EXPECT_TRUE(serializer->Serialize(original, file));
        fclose(file);

        concept_test_types::SimplePod restored{};
        file = fopen(test_file_.c_str(), "rb");
        ASSERT_NE(file, nullptr);
        EXPECT_TRUE(serializer->Deserialize(restored, file));
        fclose(file);

        EXPECT_EQ(original, restored);
    }

    {
        concept_test_types::WithMethods original{100};
        auto serializer = CreateSerializer<concept_test_types::WithMethods>();

        FILE* file = fopen(test_file_.c_str(), "wb");
        ASSERT_NE(file, nullptr);
        EXPECT_TRUE(serializer->Serialize(original, file));
        fclose(file);

        concept_test_types::WithMethods restored{};
        file = fopen(test_file_.c_str(), "rb");
        ASSERT_NE(file, nullptr);
        EXPECT_TRUE(serializer->Deserialize(restored, file));
        fclose(file);

        EXPECT_EQ(original, restored);
    }

    {
        concept_test_types::OnlyAdlFunctions original{3.14159};
        auto serializer = CreateSerializer<concept_test_types::OnlyAdlFunctions>();

        FILE* file = fopen(test_file_.c_str(), "wb");
        ASSERT_NE(file, nullptr);
        EXPECT_TRUE(serializer->Serialize(original, file));
        fclose(file);

        concept_test_types::OnlyAdlFunctions restored{};
        file = fopen(test_file_.c_str(), "rb");
        ASSERT_NE(file, nullptr);
        EXPECT_TRUE(serializer->Deserialize(restored, file));
        fclose(file);

        EXPECT_EQ(original, restored);
    }
}

/**
 * @brief Test for std::string and std::vector specializations
 */
TEST_F(ConceptValidationTest, SpecializedSerializers) {
    {
        std::string original = "Hello, World! Тест unicode строки 🚀";

        auto serializer = CreateSerializer<std::string>();

        FILE* file = fopen(test_file_.c_str(), "wb");
        ASSERT_NE(file, nullptr);
        EXPECT_TRUE(serializer->Serialize(original, file));
        fclose(file);

        std::string restored;
        file = fopen(test_file_.c_str(), "rb");
        ASSERT_NE(file, nullptr);
        EXPECT_TRUE(serializer->Deserialize(restored, file));
        fclose(file);

        EXPECT_EQ(original, restored);
    }

    {
        std::vector<int> original = {1, 2, 3, 4, 5, -1, -2, -3};

        auto serializer = CreateSerializer<std::vector<int>>();

        FILE* file = fopen(test_file_.c_str(), "wb");
        ASSERT_NE(file, nullptr);
        EXPECT_TRUE(serializer->Serialize(original, file));
        fclose(file);

        std::vector<int> restored;
        file = fopen(test_file_.c_str(), "rb");
        ASSERT_NE(file, nullptr);
        EXPECT_TRUE(serializer->Deserialize(restored, file));
        fclose(file);

        EXPECT_EQ(original, restored);
    }

    {
        std::vector<std::vector<double>> original = {{1.1, 2.2, 3.3}, {}, {4.4, 5.5}, {6.6}};

        auto serializer = CreateSerializer<std::vector<std::vector<double>>>();

        FILE* file = fopen(test_file_.c_str(), "wb");
        ASSERT_NE(file, nullptr);
        EXPECT_TRUE(serializer->Serialize(original, file));
        fclose(file);

        std::vector<std::vector<double>> restored;
        file = fopen(test_file_.c_str(), "rb");
        ASSERT_NE(file, nullptr);
        EXPECT_TRUE(serializer->Deserialize(restored, file));
        fclose(file);

        EXPECT_EQ(original, restored);
    }
}

/**
 * @brief Test for complex nested structure with multiple types
 */
TEST_F(ConceptValidationTest, ComplexNestedStructure) {
    concept_test_types::ComplexData original(42,                                   // integer_value
                                             3.14159,                              // double_value
                                             "Complex data with unicode: αβγ 🚀",  // text
                                             100  // nested_object value
    );

    auto serializer = CreateSerializer<concept_test_types::ComplexData>();

    FILE* file = fopen(test_file_.c_str(), "wb");
    ASSERT_NE(file, nullptr);
    EXPECT_TRUE(serializer->Serialize(original, file));
    fclose(file);

    concept_test_types::ComplexData restored;
    file = fopen(test_file_.c_str(), "rb");
    ASSERT_NE(file, nullptr);
    EXPECT_TRUE(serializer->Deserialize(restored, file));
    fclose(file);

    EXPECT_EQ(original, restored);
    EXPECT_EQ(original.integer_value, restored.integer_value);
    EXPECT_DOUBLE_EQ(original.double_value, restored.double_value);
    EXPECT_EQ(original.text, restored.text);
    EXPECT_EQ(original.nested_object.value, restored.nested_object.value);
}
