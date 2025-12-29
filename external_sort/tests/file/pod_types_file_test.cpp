/**
 * @file pod_types_file_test.cpp
 * @brief Tests for sorting POD types with file storage
 */

#include "k_way_merge_sorter.hpp"
#include "file_stream.hpp"

#include <algorithm>
#include <filesystem>
#include <gtest/gtest.h>
#include <vector>

using external_sort::KWayMergeSorter;

/**
 * @brief Basic template class for testing POD types with files
 * @tparam T Type of data being tested
 */
template <typename T>
class PodTypesFileTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = "test_pod_file_sort_" + std::to_string(test_counter++);
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
        std::filesystem::create_directory(test_dir_);
        factory_ = std::make_unique<io::FileStreamFactory<T>>(test_dir_);
    }

    void TearDown() override {
        factory_.reset();
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
    }

    std::string test_dir_;
    std::unique_ptr<io::FileStreamFactory<T>> factory_;
    static int test_counter;

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
};

template <typename T>
int PodTypesFileTest<T>::test_counter = 0;

/**
 * @brief Testing types with files: int32_t
 */
class Int32FileTest : public PodTypesFileTest<int32_t> {};

TEST_F(Int32FileTest, BasicSorting) {
    const std::string input_file = (std::filesystem::path(test_dir_) / "input.bin").string();
    const std::string output_file = (std::filesystem::path(test_dir_) / "output.bin").string();

    std::vector<int32_t> test_data = {2'147'483'647, -2'147'483'648, 0,
                                      1'000'000,     -1'000'000,     123'456'789};
    CreateTestData(input_file, test_data);

    KWayMergeSorter<int32_t> sorter(*factory_, input_file, output_file, sizeof(int32_t) * 3, 2, 10,
                                    true);
    sorter.Sort();

    std::vector<int32_t> result = ReadAllData(output_file);
    std::vector<int32_t> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result, expected);
}

/**
 * @brief Testing types with files: uint64_t
 */
class UInt64FileTest : public PodTypesFileTest<uint64_t> {};

TEST_F(UInt64FileTest, BasicSorting) {
    const std::string input_file = (std::filesystem::path(test_dir_) / "input.bin").string();
    const std::string output_file = (std::filesystem::path(test_dir_) / "output.bin").string();

    std::vector<uint64_t> test_data = {18'446'744'073'709'551'615ULL, 0,
                                       9'223'372'036'854'775'808ULL,  1,
                                       1'000'000'000'000ULL,          123'456'789'012'345ULL};
    CreateTestData(input_file, test_data);

    KWayMergeSorter<uint64_t> sorter(*factory_, input_file, output_file, sizeof(uint64_t) * 3, 2,
                                     10, true);
    sorter.Sort();

    std::vector<uint64_t> result = ReadAllData(output_file);
    std::vector<uint64_t> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result, expected);
}

/**
 * @brief Testing types with files: double
 */
class DoubleFileTest : public PodTypesFileTest<double> {};

TEST_F(DoubleFileTest, BasicSorting) {
    const std::string input_file = (std::filesystem::path(test_dir_) / "input.bin").string();
    const std::string output_file = (std::filesystem::path(test_dir_) / "output.bin").string();

    std::vector<double> test_data = {3.14159, -2.71828, 0.0, 1.41421, -1.73205, 2.23607};
    CreateTestData(input_file, test_data);

    KWayMergeSorter<double> sorter(*factory_, input_file, output_file, sizeof(double) * 3, 2, 10,
                                   true);
    sorter.Sort();

    std::vector<double> result = ReadAllData(output_file);
    std::vector<double> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result, expected);
}
