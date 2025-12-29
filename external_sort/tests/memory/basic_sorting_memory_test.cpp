/**
 * @file basic_sorting_memory_test.cpp
 * @brief Basic tests for sorter with in-memory storage
 */

#include "k_way_merge_sorter.hpp"
#include "memory_stream.hpp"

#include <algorithm>
#include <gtest/gtest.h>
#include <random>
#include <vector>

using external_sort::KWayMergeSorter;

/**
 * @brief Fixture for KWayMergeSorter tests with in-memory storage
 */
class BasicSortingMemoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        factory_ = std::make_unique<io::InMemoryStreamFactory<int>>();
    }

    void TearDown() override {
        factory_.reset();
    }

    std::unique_ptr<io::InMemoryStreamFactory<int>> factory_;

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
            result.push_back(input->Value());
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
 * @brief Test simple sorting of small array
 */
TEST_F(BasicSortingMemoryTest, SimpleSmallArraySort) {
    const std::string input_id = "small_input";
    const std::string output_id = "small_output";
    const std::vector<int> test_data = {5, 2, 8, 1, 9, 3};

    CreateTestData(input_id, test_data);

    KWayMergeSorter<int> sorter(*factory_, input_id, output_id, sizeof(int) * 3, 2, 10, true);
    sorter.Sort();

    std::vector<int> result = ReadAllData(output_id);
    std::vector<int> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, true));
}

/**
 * @brief Test descending sort
 */
TEST_F(BasicSortingMemoryTest, DescendingSort) {
    const std::string input_id = "desc_input";
    const std::string output_id = "desc_output";
    const std::vector<int> test_data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    CreateTestData(input_id, test_data);

    KWayMergeSorter<int> sorter(*factory_, input_id, output_id, sizeof(int) * 4, 3, 10, false);
    sorter.Sort();

    std::vector<int> result = ReadAllData(output_id);
    std::vector<int> expected = test_data;
    std::sort(expected.begin(), expected.end(), std::greater<int>());

    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, false));
}

/**
 * @brief Test sorting empty input
 */
TEST_F(BasicSortingMemoryTest, EmptyInputSort) {
    const std::string input_id = "empty_input";
    const std::string output_id = "empty_output";

    CreateTestData(input_id, {});

    KWayMergeSorter<int> sorter(*factory_, input_id, output_id, sizeof(int) * 10, 2, 10, true);

    EXPECT_NO_THROW(sorter.Sort());  // NOLINT(cppcoreguidelines-avoid-goto)

    std::vector<int> result = ReadAllData(output_id);
    EXPECT_TRUE(result.empty());
}

/**
 * @brief Test sorting single element array
 */
TEST_F(BasicSortingMemoryTest, SingleElementSort) {
    const std::string input_id = "single_input";
    const std::string output_id = "single_output";
    const std::vector<int> test_data = {42};

    CreateTestData(input_id, test_data);

    KWayMergeSorter<int> sorter(*factory_, input_id, output_id, sizeof(int) * 10, 2, 10, true);
    sorter.Sort();

    std::vector<int> result = ReadAllData(output_id);
    EXPECT_EQ(result, test_data);
}

/**
 * @brief Test large data with forced creation of multiple runs
 */
TEST_F(BasicSortingMemoryTest, LargeDataMultipleRuns) {
    const std::string input_id = "large_input";
    const std::string output_id = "large_output";

    std::vector<int> test_data = GenerateRandomData(100, 0, 1000);
    CreateTestData(input_id, test_data);

    KWayMergeSorter<int> sorter(*factory_, input_id, output_id, sizeof(int) * 10, 4, 20, true);
    sorter.Sort();

    std::vector<int> result = ReadAllData(output_id);
    std::vector<int> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result.size(), expected.size());
    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, true));
}

/**
 * @brief Test already sorted data
 */
TEST_F(BasicSortingMemoryTest, AlreadySortedData) {
    const std::string input_id = "sorted_input";
    const std::string output_id = "sorted_output";

    std::vector<int> test_data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    CreateTestData(input_id, test_data);

    KWayMergeSorter<int> sorter(*factory_, input_id, output_id, sizeof(int) * 5, 2, 10, true);
    sorter.Sort();

    std::vector<int> result = ReadAllData(output_id);
    EXPECT_EQ(result, test_data);
}

/**
 * @brief Test reverse sorted data
 */
TEST_F(BasicSortingMemoryTest, ReverseSortedData) {
    const std::string input_id = "reverse_input";
    const std::string output_id = "reverse_output";

    std::vector<int> test_data = {10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
    CreateTestData(input_id, test_data);

    KWayMergeSorter<int> sorter(*factory_, input_id, output_id, sizeof(int) * 5, 2, 10, true);
    sorter.Sort();

    std::vector<int> result = ReadAllData(output_id);
    std::vector<int> expected = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    EXPECT_EQ(result, expected);
}

/**
 * @brief Test data with duplicates
 */
TEST_F(BasicSortingMemoryTest, DataWithDuplicates) {
    const std::string input_id = "dup_input";
    const std::string output_id = "dup_output";

    std::vector<int> test_data = {3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5};
    CreateTestData(input_id, test_data);

    KWayMergeSorter<int> sorter(*factory_, input_id, output_id, sizeof(int) * 4, 3, 10, true);
    sorter.Sort();

    std::vector<int> result = ReadAllData(output_id);
    std::vector<int> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, true));
}

/**
 * @brief Test different K values
 */
TEST_F(BasicSortingMemoryTest, DifferentKValues) {
    const std::vector<int> test_data = GenerateRandomData(50, 0, 100);

    for (uint64_t k = 2; k <= 8; k += 2) {
        const std::string input_id = "k_test_input_" + std::to_string(k);
        const std::string output_id = "k_test_output_" + std::to_string(k);

        CreateTestData(input_id, test_data);

        KWayMergeSorter<int> sorter(*factory_, input_id, output_id, sizeof(int) * 8, k, 10, true);

        EXPECT_NO_THROW(sorter.Sort())  // NOLINT(cppcoreguidelines-avoid-goto)
            << "Error at K=" << k;

        std::vector<int> result = ReadAllData(output_id);
        std::vector<int> expected = test_data;
        std::sort(expected.begin(), expected.end());

        EXPECT_EQ(result, expected) << "Incorrect result at K=" << k;
        EXPECT_TRUE(IsSorted(result, true)) << "Не отсортировано при K=" << k;
    }
}

/**
 * @brief Test error with invalid K
 */
TEST_F(BasicSortingMemoryTest, InvalidKValue) {
    const std::string input_id = "invalid_k_input";
    const std::string output_id = "invalid_k_output";

    CreateTestData(input_id, {1, 2, 3});

    EXPECT_THROW(  // NOLINT(cppcoreguidelines-avoid-goto)
        {
            KWayMergeSorter<int> sorter(*factory_, input_id, output_id, sizeof(int) * 10, 1, 10,
                                        true);
        },
        std::invalid_argument);

    EXPECT_THROW(  // NOLINT(cppcoreguidelines-avoid-goto)
        {
            KWayMergeSorter<int> sorter(*factory_, input_id, output_id, sizeof(int) * 10, 0, 10,
                                        true);
        },
        std::invalid_argument);
}

/**
 * @brief Test error with too small memory limit
 */
TEST_F(BasicSortingMemoryTest, TooSmallMemoryLimit) {
    const std::string input_id = "small_mem_input";
    const std::string output_id = "small_mem_output";

    CreateTestData(input_id, {1, 2, 3});

    EXPECT_THROW(  // NOLINT(cppcoreguidelines-avoid-goto)
        {
            KWayMergeSorter<int> sorter(*factory_, input_id, output_id, 1, 2, 10, true);
            sorter.Sort();
        },
        std::runtime_error);
}

/**
 * @brief Stress test with large data
 */
TEST_F(BasicSortingMemoryTest, StressTestLargeData) {
    const std::string input_id = "stress_input";
    const std::string output_id = "stress_output";

    const uint64_t data_size = 1000;
    std::vector<int> test_data = GenerateRandomData(data_size, 0, 10'000);
    CreateTestData(input_id, test_data);

    KWayMergeSorter<int> sorter(*factory_, input_id, output_id, sizeof(int) * 20, 8, 50, true);

    auto start = std::chrono::high_resolution_clock::now();
    sorter.Sort();
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Stress test completed in " << duration.count() << " ms" << std::endl;

    std::vector<int> result = ReadAllData(output_id);
    std::vector<int> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result.size(), expected.size());
    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, true));
}
