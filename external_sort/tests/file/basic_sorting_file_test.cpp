/**
 * @file basic_sorting_file_test.cpp
 * @brief Basic tests for sorter with file storage
 */

#include "k_way_merge_sorter.hpp"
#include "file_stream.hpp"

#include <algorithm>
#include <filesystem>
#include <gtest/gtest.h>
#include <random>
#include <vector>

using external_sort::KWayMergeSorter;

/**
 * @brief Fixture for KWayMergeSorter tests with file storage
 */
class BasicSortingFileTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = "test_basic_file_sort";
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
        std::filesystem::create_directory(test_dir_);

        factory_ = std::make_unique<io::FileStreamFactory<int>>(test_dir_);
    }

    void TearDown() override {
        factory_.reset();
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
    }

    std::string test_dir_;
    std::unique_ptr<io::FileStreamFactory<int>> factory_;

    void CreateTestData(const std::string& storage_id, const std::vector<int>& data) {
        auto output = factory_->CreateOutputStream(storage_id, 100);
        for (int value : data) {
            output->Write(value);
        }
        output->Finalize();
    }

    std::vector<int> ReadAllData(const std::string& storage_id) {
        auto input = factory_->CreateInputStream(storage_id, 100);
        std::vector<int> result;
        while (!input->IsExhausted()) {
            result.push_back(input->TakeValue());
            input->Advance();
        }
        return result;
    }

    std::vector<int> GenerateRandomData(uint64_t size, int min_val = 0, int max_val = 1000) {
        std::vector<int> data;
        data.reserve(size);

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dis(min_val, max_val);

        for (uint64_t i = 0; i < size; ++i) {
            data.push_back(dis(gen));
        }
        return data;
    }

    bool IsSorted(const std::vector<int>& data, bool ascending = true) {
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
 * @brief Test simple sorting with files
 */
TEST_F(BasicSortingFileTest, SimpleFileSorting) {
    const std::string input_file = (std::filesystem::path(test_dir_) / "input.bin").string();
    const std::string output_file = (std::filesystem::path(test_dir_) / "output.bin").string();

    std::vector<int> test_data = {9, 2, 7, 4, 1, 8, 3, 6, 5};
    CreateTestData(input_file, test_data);

    KWayMergeSorter<int> sorter(*factory_, input_file, output_file, sizeof(int) * 4, 3, 10, true);
    sorter.Sort();

    std::vector<int> result = ReadAllData(output_file);
    std::vector<int> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result, expected);
    EXPECT_TRUE(std::filesystem::exists(output_file));
}

/**
 * @brief Test sorting large data with files
 */
TEST_F(BasicSortingFileTest, LargeDataFileSorting) {
    const std::string input_file = (std::filesystem::path(test_dir_) / "large_input.bin").string();
    const std::string output_file =
        (std::filesystem::path(test_dir_) / "large_output.bin").string();

    std::vector<int> test_data = GenerateRandomData(500, 0, 10000);
    CreateTestData(input_file, test_data);

    KWayMergeSorter<int> sorter(*factory_, input_file, output_file, sizeof(int) * 20, 4, 50, true);
    sorter.Sort();

    std::vector<int> result = ReadAllData(output_file);
    std::vector<int> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, true));
}

/**
 * @brief Test descending sort with files
 */
TEST_F(BasicSortingFileTest, DescendingSortFile) {
    const std::string input_file = (std::filesystem::path(test_dir_) / "desc_input.bin").string();
    const std::string output_file = (std::filesystem::path(test_dir_) / "desc_output.bin").string();

    std::vector<int> test_data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    CreateTestData(input_file, test_data);

    KWayMergeSorter<int> sorter(*factory_, input_file, output_file, sizeof(int) * 4, 3, 10, false);
    sorter.Sort();

    std::vector<int> result = ReadAllData(output_file);
    std::vector<int> expected = test_data;
    std::sort(expected.begin(), expected.end(), std::greater<int>());

    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, false));
}

/**
 * @brief Test empty input file
 */
TEST_F(BasicSortingFileTest, EmptyFileSort) {
    const std::string input_file = (std::filesystem::path(test_dir_) / "empty_input.bin").string();
    const std::string output_file =
        (std::filesystem::path(test_dir_) / "empty_output.bin").string();

    CreateTestData(input_file, {});

    KWayMergeSorter<int> sorter(*factory_, input_file, output_file, sizeof(int) * 10, 2, 10, true);

    EXPECT_NO_THROW(sorter.Sort());  // NOLINT(cppcoreguidelines-avoid-goto)

    std::vector<int> result = ReadAllData(output_file);
    EXPECT_TRUE(result.empty());
}
