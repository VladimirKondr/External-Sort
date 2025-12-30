/**
 * @file string_sorting_test.cpp
 * @brief Tests for sorting std::string strings
 */

#include "k_way_merge_sorter.hpp"
#include "file_stream.hpp"

#include <algorithm>
#include <filesystem>
#include <gtest/gtest.h>
#include <string>
#include <vector>

/**
 * @brief Fixture for string sorting tests
 */
class StringSortingTest : public ::testing::Test {
protected:
    std::unique_ptr<io::FileStreamFactory<std::string>> factory_;
    std::string test_dir_;
    static int test_counter;

    void SetUp() override {
        test_counter++;
        test_dir_ = "string_test_" + std::to_string(test_counter);

        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
        std::filesystem::create_directory(test_dir_);

        factory_ = std::make_unique<io::FileStreamFactory<std::string>>(test_dir_);
    }

    void TearDown() override {
        factory_.reset();
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
    }

    void CreateTestData(const std::string& storage_id, const std::vector<std::string>& data) {
        auto output = factory_->CreateOutputStream(storage_id, 100);
        for (const auto& item : data) {
            output->Write(item);
        }
        output->Finalize();
    }

    std::vector<std::string> ReadAllData(const std::string& storage_id) {
        auto input = factory_->CreateInputStream(storage_id, 100);
        std::vector<std::string> result;
        while (!input->IsExhausted()) {
            result.push_back(input->TakeValue());
            input->Advance();
        }
        return result;
    }

    bool IsSorted(const std::vector<std::string>& data, bool ascending = true) {
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

int StringSortingTest::test_counter = 0;

/**
 * @brief Test basic string sorting
 */
TEST_F(StringSortingTest, BasicStringSorting) {
    const std::string input_id = "basic_input";
    const std::string output_id = "basic_output";

    std::vector<std::string> test_data = {"zebra", "apple", "banana", "cherry", "date"};

    CreateTestData(input_id, test_data);

    external_sort::KWayMergeSorter<std::string> sorter(*factory_, input_id, output_id, 1024, 2, 10,
                                                       true);
    sorter.Sort();

    std::vector<std::string> result = ReadAllData(output_id);
    std::vector<std::string> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, true));
}

/**
 * @brief Test descending string sorting
 */
TEST_F(StringSortingTest, DescendingStringSorting) {
    const std::string input_id = "desc_input";
    const std::string output_id = "desc_output";

    std::vector<std::string> test_data = {"apple", "banana", "cherry", "date", "elderberry"};

    CreateTestData(input_id, test_data);

    external_sort::KWayMergeSorter<std::string> sorter(*factory_, input_id, output_id, 1024, 2, 10,
                                                       false);
    sorter.Sort();

    std::vector<std::string> result = ReadAllData(output_id);
    std::vector<std::string> expected = test_data;
    std::sort(expected.rbegin(), expected.rend());

    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, false));
}

/**
 * @brief Test sorting strings of variable lengths
 */
TEST_F(StringSortingTest, VariableLengthStrings) {
    const std::string input_id = "var_length_input";
    const std::string output_id = "var_length_output";

    std::vector<std::string> test_data = {"a", "very_long_string", "xyz", "medium", "bb"};

    CreateTestData(input_id, test_data);

    external_sort::KWayMergeSorter<std::string> sorter(*factory_, input_id, output_id, 4096, 2, 10,
                                                       true);
    sorter.Sort();

    std::vector<std::string> result = ReadAllData(output_id);
    std::vector<std::string> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, true));
}

/**
 * @brief Test sorting strings with empty strings
 */
TEST_F(StringSortingTest, EmptyStrings) {
    const std::string input_id = "empty_input";
    const std::string output_id = "empty_output";

    std::vector<std::string> test_data = {"", "zebra", "", "apple", ""};

    CreateTestData(input_id, test_data);

    external_sort::KWayMergeSorter<std::string> sorter(*factory_, input_id, output_id, 1024, 2, 10,
                                                       true);
    sorter.Sort();

    std::vector<std::string> result = ReadAllData(output_id);
    std::vector<std::string> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, true));
}

/**
 * @brief Test sorting identical strings
 */
TEST_F(StringSortingTest, DuplicateStrings) {
    const std::string input_id = "duplicate_input";
    const std::string output_id = "duplicate_output";

    std::vector<std::string> test_data = {"apple", "banana", "apple", "cherry", "banana", "apple"};

    CreateTestData(input_id, test_data);

    external_sort::KWayMergeSorter<std::string> sorter(*factory_, input_id, output_id, 1024, 2, 10,
                                                       true);
    sorter.Sort();

    std::vector<std::string> result = ReadAllData(output_id);
    std::vector<std::string> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, true));
}
