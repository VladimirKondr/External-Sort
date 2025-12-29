/**
 * @file pod_types_memory_test.cpp
 * @brief Tests for sorting POD types with in-memory storage
 */

#include "k_way_merge_sorter.hpp"
#include "memory_stream.hpp"

#include <algorithm>
#include <gtest/gtest.h>
#include <vector>

using external_sort::KWayMergeSorter;

/**
 * @brief Basic template class for testing POD types
 * @tparam T Type of data being tested
 */
template <typename T>
class PodTypesMemoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        factory_ = std::make_unique<io::InMemoryStreamFactory<T>>();
    }

    void TearDown() override {
        factory_.reset();
    }

    std::unique_ptr<io::InMemoryStreamFactory<T>> factory_;

    void CreateTestData(const std::string& storage_id, const std::vector<T>& data) {
        auto output = factory_->CreateOutputStream(storage_id, 100);
        for (const T& value : data) {
            output->Write(value);
        }
        output->Finalize();
    }

    std::vector<T> ReadAllData(const std::string& storage_id) {
        auto input = factory_->CreateInputStream(storage_id, 100);
        std::vector<T> result;
        while (!input->IsExhausted()) {
            result.push_back(input->Value());
            input->Advance();
        }
        return result;
    }

    bool IsSorted(const std::vector<T>& data, bool ascending = true) {
        if (data.size() <= 1) {
            return true;
        }

        for (uint64_t i = 1; i < data.size(); ++i) {
            if (ascending && data[i] < data[i - 1]) {
                return false;
            }
            if (!ascending && data[i] > data[i - 1]) {
                return false;
            }
        }
        return true;
    }
};

/**
 * @brief Testing with int8_t type (8-bit signed numbers)
 */
class Int8MemoryTest : public PodTypesMemoryTest<int8_t> {};

TEST_F(Int8MemoryTest, BasicSorting) {
    const std::string input_id = "int8_input";
    const std::string output_id = "int8_output";

    std::vector<int8_t> test_data = {127, -128, 0, 42, -1, 100, -50};
    CreateTestData(input_id, test_data);

    external_sort::KWayMergeSorter<int8_t> sorter(*factory_, input_id, output_id,
                                                  sizeof(int8_t) * 4, 2, 10, true);
    sorter.Sort();

    std::vector<int8_t> result = ReadAllData(output_id);
    std::vector<int8_t> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, true));
}

/**
 * @brief Testing with uint8_t type (8-bit unsigned numbers)
 */
class UInt8MemoryTest : public PodTypesMemoryTest<uint8_t> {};

TEST_F(UInt8MemoryTest, BasicSorting) {
    const std::string input_id = "uint8_input";
    const std::string output_id = "uint8_output";

    std::vector<uint8_t> test_data = {255, 0, 128, 1, 127, 200, 50};
    CreateTestData(input_id, test_data);

    external_sort::KWayMergeSorter<uint8_t> sorter(*factory_, input_id, output_id,
                                                   sizeof(uint8_t) * 4, 2, 10, true);
    sorter.Sort();

    std::vector<uint8_t> result = ReadAllData(output_id);
    std::vector<uint8_t> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, true));
}

/**
 * @brief Testing with int16_t type (16-bit signed numbers)
 */
class Int16MemoryTest : public PodTypesMemoryTest<int16_t> {};

TEST_F(Int16MemoryTest, BasicSorting) {
    const std::string input_id = "int16_input";
    const std::string output_id = "int16_output";

    std::vector<int16_t> test_data = {32'767, -32'768, 0, 1000, -1000, 12'345, -5432};
    CreateTestData(input_id, test_data);

    external_sort::KWayMergeSorter<int16_t> sorter(*factory_, input_id, output_id,
                                                   sizeof(int16_t) * 4, 2, 10, true);
    sorter.Sort();

    std::vector<int16_t> result = ReadAllData(output_id);
    std::vector<int16_t> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, true));
}

/**
 * @brief Testing with uint16_t type (16-bit unsigned numbers)
 */
class UInt16MemoryTest : public PodTypesMemoryTest<uint16_t> {};

TEST_F(UInt16MemoryTest, BasicSorting) {
    const std::string input_id = "uint16_input";
    const std::string output_id = "uint16_output";

    std::vector<uint16_t> test_data = {65'535, 0, 32'768, 1, 12'345, 54'321, 9999};
    CreateTestData(input_id, test_data);

    external_sort::KWayMergeSorter<uint16_t> sorter(*factory_, input_id, output_id,
                                                    sizeof(uint16_t) * 4, 2, 10, true);
    sorter.Sort();

    std::vector<uint16_t> result = ReadAllData(output_id);
    std::vector<uint16_t> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, true));
}

/**
 * @brief Testing with int32_t type (32-bit signed numbers)
 */
class Int32MemoryTest : public PodTypesMemoryTest<int32_t> {};

TEST_F(Int32MemoryTest, BasicSorting) {
    const std::string input_id = "int32_input";
    const std::string output_id = "int32_output";

    std::vector<int32_t> test_data = {2'147'483'647, -2'147'483'648, 0,           1'000'000,
                                      -1'000'000,    123'456'789,    -987'654'321};
    CreateTestData(input_id, test_data);

    KWayMergeSorter<int32_t> sorter(*factory_, input_id, output_id, sizeof(int32_t) * 4, 2, 10,
                                    true);
    sorter.Sort();

    std::vector<int32_t> result = ReadAllData(output_id);
    std::vector<int32_t> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, true));
}

/**
 * @brief Testing with uint32_t type (32-bit unsigned numbers)
 */
class UInt32MemoryTest : public PodTypesMemoryTest<uint32_t> {};

TEST_F(UInt32MemoryTest, BasicSorting) {
    const std::string input_id = "uint32_input";
    const std::string output_id = "uint32_output";

    std::vector<uint32_t> test_data = {4'294'967'295U, 0,           2'147'483'648U, 1,
                                       1'000'000'000U, 123'456'789, 987'654'321};
    CreateTestData(input_id, test_data);

    KWayMergeSorter<uint32_t> sorter(*factory_, input_id, output_id, sizeof(uint32_t) * 4, 2, 10,
                                     true);
    sorter.Sort();

    std::vector<uint32_t> result = ReadAllData(output_id);
    std::vector<uint32_t> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, true));
}

/**
 * @brief Testing with int64_t type (64-bit signed numbers)
 */
class Int64MemoryTest : public PodTypesMemoryTest<int64_t> {};

TEST_F(Int64MemoryTest, BasicSorting) {
    const std::string input_id = "int64_input";
    const std::string output_id = "int64_output";

    std::vector<int64_t> test_data = {9'223'372'036'854'775'807LL,
                                      (-9'223'372'036'854'775'807LL - 1),
                                      0,
                                      1'000'000'000'000LL,
                                      -1'000'000'000'000LL,
                                      123'456'789'012'345LL,
                                      -987'654'321'098'765LL};
    CreateTestData(input_id, test_data);

    KWayMergeSorter<int64_t> sorter(*factory_, input_id, output_id, sizeof(int64_t) * 4, 2, 10,
                                    true);
    sorter.Sort();

    std::vector<int64_t> result = ReadAllData(output_id);
    std::vector<int64_t> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, true));
}

/**
 * @brief Testing with uint64_t type (64-bit unsigned numbers)
 */
class UInt64MemoryTest : public PodTypesMemoryTest<uint64_t> {};

TEST_F(UInt64MemoryTest, BasicSorting) {
    const std::string input_id = "uint64_input";
    const std::string output_id = "uint64_output";

    std::vector<uint64_t> test_data = {18'446'744'073'709'551'615ULL, 0,
                                       9'223'372'036'854'775'808ULL,  1,
                                       1'000'000'000'000ULL,          123'456'789'012'345ULL,
                                       987'654'321'098'765ULL};
    CreateTestData(input_id, test_data);

    KWayMergeSorter<uint64_t> sorter(*factory_, input_id, output_id, sizeof(uint64_t) * 4, 2, 10,
                                     true);
    sorter.Sort();

    std::vector<uint64_t> result = ReadAllData(output_id);
    std::vector<uint64_t> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, true));
}

/**
 * @brief Testing with float type (32-bit floating point numbers)
 */
class FloatMemoryTest : public PodTypesMemoryTest<float> {};

TEST_F(FloatMemoryTest, BasicSorting) {
    const std::string input_id = "float_input";
    const std::string output_id = "float_output";

    std::vector<float> test_data = {3.14159f,  -2.71828f, 0.0f,     1.41421f,
                                    -1.73205f, 2.23607f,  -3.16227f};
    CreateTestData(input_id, test_data);

    KWayMergeSorter<float> sorter(*factory_, input_id, output_id, sizeof(float) * 4, 2, 10, true);
    sorter.Sort();

    std::vector<float> result = ReadAllData(output_id);
    std::vector<float> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, true));
}

/**
 * @brief Testing with double type (64-bit floating point numbers)
 */
class DoubleMemoryTest : public PodTypesMemoryTest<double> {};

TEST_F(DoubleMemoryTest, BasicSorting) {
    const std::string input_id = "double_input";
    const std::string output_id = "double_output";

    std::vector<double> test_data = {3.141592653589793,  -2.718281828459045,  0.0,
                                     1.4142135623730951, -1.7320508075688772, 2.2360679774997896,
                                     -3.1622776601683795};
    CreateTestData(input_id, test_data);

    KWayMergeSorter<double> sorter(*factory_, input_id, output_id, sizeof(double) * 4, 2, 10, true);
    sorter.Sort();

    std::vector<double> result = ReadAllData(output_id);
    std::vector<double> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, true));
}

/**
 * @brief Simple POD structure for testing
 */
struct SimplePodStruct {
    int32_t id;
    float value;

    bool operator<(const SimplePodStruct& other) const {
        return id < other.id;
    }

    bool operator>(const SimplePodStruct& other) const {
        return id > other.id;
    }

    bool operator==(const SimplePodStruct& other) const {
        return id == other.id && value == other.value;
    }

    bool operator!=(const SimplePodStruct& other) const {
        return !(*this == other);
    }
};

/**
 * @brief Testing with custom POD type
 */
class CustomPodStructMemoryTest : public PodTypesMemoryTest<SimplePodStruct> {};

TEST_F(CustomPodStructMemoryTest, BasicSorting) {
    const std::string input_id = "struct_input";
    const std::string output_id = "struct_output";

    std::vector<SimplePodStruct> test_data = {{5, 3.14f}, {2, 2.71f}, {8, 1.41f}, {1, 1.73f},
                                              {7, 2.23f}, {3, 3.16f}, {6, 1.61f}};
    CreateTestData(input_id, test_data);

    KWayMergeSorter<SimplePodStruct> sorter(*factory_, input_id, output_id,
                                            sizeof(SimplePodStruct) * 4, 2, 10, true);
    sorter.Sort();

    std::vector<SimplePodStruct> result = ReadAllData(output_id);
    std::vector<SimplePodStruct> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, true));
}
