/**
 * @file type_concepts_test.cpp
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

class WithWrongMethods {
public:
    int value;

    void Serialize(FILE* file) const {
        fwrite(&value, sizeof(int), 1, file);
    }

    void Deserialize(FILE* file) {
        fread(&value, sizeof(int), 1, file);
    }
};

class WithWrongParameters {
public:
    int value;

    bool Serialize(int) const {
        return true;
    }

    bool Deserialize(int) {
        return true;
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
 * @brief Test for PodSerializable concept
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
 * @brief Test for MethodSerializable concept
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
 * @brief Test for CustomSerializable concept
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
 * @brief Test for FileSerializable concept
 */
TEST_F(ConceptValidationTest, FileSerializableConcept) {
    static_assert(FileSerializable<int>);
    static_assert(FileSerializable<concept_test_types::SimplePod>);
    static_assert(FileSerializable<concept_test_types::WithMethods>);
    static_assert(FileSerializable<concept_test_types::OnlyAdlFunctions>);
    static_assert(FileSerializable<concept_test_types::WithAdlFunctions>);
    static_assert(FileSerializable<std::string>);
    static_assert(FileSerializable<std::vector<int>>);

    static_assert(!serialization::SpecializedSerializable<concept_test_types::NonSerializable>);
    static_assert(!FileSerializable<concept_test_types::NonSerializable>);
}

/**
 * @brief Test for compilation errors with non-serializable types
 */
TEST_F(ConceptValidationTest, CompilationErrors) {
    static_assert(!FileSerializable<concept_test_types::NonSerializable>);
    static_assert(!FileSerializable<std::unique_ptr<int>>);

    SUCCEED();
}
