/**
 * @file test_string_sorting.cpp
 * @brief Комплексные тесты сортировки строк std::string с помощью библиотеки внешней сортировки
 * @author External Sort Library
 * @version 1.0
 */

#include "file_stream.hpp"
#include "k_way_merge_sorter.hpp"
#include "memory_stream.hpp"

#include <algorithm>
#include <filesystem>
#include <gtest/gtest.h>
#include <random>
#include <string>
#include <vector>

using namespace external_sort;

/**
 * @brief Базовый класс для тестов сортировки строк с файловым хранилищем
 */
class StringSortingTestBase : public ::testing::Test {
   protected:
    std::unique_ptr<FileStreamFactory<std::string>> factory;
    std::string test_dir;
    static int test_counter;

    void SetUp() override {
        test_counter++;
        test_dir = "string_test_" + std::to_string(test_counter);

        if (std::filesystem::exists(test_dir)) {
            std::filesystem::remove_all(test_dir);
        }
        std::filesystem::create_directory(test_dir);

        factory = std::make_unique<FileStreamFactory<std::string>>(test_dir);
    }

    void TearDown() override {
        factory.reset();
        if (std::filesystem::exists(test_dir)) {
            std::filesystem::remove_all(test_dir);
        }
    }

    void CreateTestData(const std::string& storage_id, const std::vector<std::string>& data) {
        auto output = factory->CreateOutputStream(storage_id, 100);
        for (const auto& item : data) {
            output->Write(item);
        }
        output->Finalize();
    }

    std::vector<std::string> ReadAllData(const std::string& storage_id) {
        auto input = factory->CreateInputStream(storage_id, 100);
        std::vector<std::string> result;
        while (!input->IsExhausted()) {
            result.push_back(input->Value());
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

int StringSortingTestBase::test_counter = 0;

/**
 * @brief Тест базовой сортировки строк
 */
TEST_F(StringSortingTestBase, BasicStringSorting) {
    const std::string input_id = "basic_input";
    const std::string output_id = "basic_output";

    std::vector<std::string> test_data = {"zebra",      "apple", "banana", "cherry",   "date",
                                          "elderberry", "fig",   "grape",  "honeydew", "kiwi"};

    CreateTestData(input_id, test_data);

    KWayMergeSorter<std::string> sorter(*factory, input_id, output_id, 1024, 2, 10, true);
    sorter.Sort();

    std::vector<std::string> result = ReadAllData(output_id);
    std::vector<std::string> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, true));
}

/**
 * @brief Тест сортировки строк по убыванию
 */
TEST_F(StringSortingTestBase, DescendingStringSorting) {
    const std::string input_id = "desc_input";
    const std::string output_id = "desc_output";

    std::vector<std::string> test_data = {"apple", "banana", "cherry", "date", "elderberry"};

    CreateTestData(input_id, test_data);

    KWayMergeSorter<std::string> sorter(*factory, input_id, output_id, 1024, 2, 10, false);
    sorter.Sort();

    std::vector<std::string> result = ReadAllData(output_id);
    std::vector<std::string> expected = test_data;
    std::sort(expected.rbegin(), expected.rend());

    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, false));
}

/**
 * @brief Тест сортировки строк с различными длинами
 */
TEST_F(StringSortingTestBase, VariableLengthStrings) {
    const std::string input_id = "var_length_input";
    const std::string output_id = "var_length_output";

    std::vector<std::string> test_data = {
      "a",   "verylongstring", "xyz", "medium", "bb", "supercalifragilisticexpialidocious", "c",
      "test"};

    CreateTestData(input_id, test_data);

    KWayMergeSorter<std::string> sorter(*factory, input_id, output_id, 4096, 2, 10, true);
    sorter.Sort();

    std::vector<std::string> result = ReadAllData(output_id);
    std::vector<std::string> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, true));
}

/**
 * @brief Тест сортировки строк с пустыми строками
 */
TEST_F(StringSortingTestBase, EmptyStrings) {
    const std::string input_id = "empty_input";
    const std::string output_id = "empty_output";

    std::vector<std::string> test_data = {"", "zebra", "", "apple", "", "banana", ""};

    CreateTestData(input_id, test_data);

    KWayMergeSorter<std::string> sorter(*factory, input_id, output_id, 1024, 2, 10, true);
    sorter.Sort();

    std::vector<std::string> result = ReadAllData(output_id);
    std::vector<std::string> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, true));
}

/**
 * @brief Тест сортировки строк с специальными символами
 */
TEST_F(StringSortingTestBase, SpecialCharacters) {
    const std::string input_id = "special_input";
    const std::string output_id = "special_output";

    std::vector<std::string> test_data = {
      "hello-world", "hello_world", "hello world", "hello.world", "hello123",
      "hello!",      "HELLO",       "Hello",       "hello"};

    CreateTestData(input_id, test_data);

    KWayMergeSorter<std::string> sorter(*factory, input_id, output_id, 1024, 2, 10, true);
    sorter.Sort();

    std::vector<std::string> result = ReadAllData(output_id);
    std::vector<std::string> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, true));
}

/**
 * @brief Тест сортировки большого количества строк
 */
TEST_F(StringSortingTestBase, LargeDataset) {
    const std::string input_id = "large_input";
    const std::string output_id = "large_output";

    std::vector<std::string> test_data;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> len_dist(1, 20);
    std::uniform_int_distribution<> char_dist('a', 'z');

    for (int i = 0; i < 1000; ++i) {
        int len = len_dist(gen);
        std::string str;
        for (int j = 0; j < len; ++j) {
            str += static_cast<char>(char_dist(gen));
        }
        test_data.push_back(str);
    }

    CreateTestData(input_id, test_data);

    KWayMergeSorter<std::string> sorter(*factory, input_id, output_id, 4096, 8, 50, true);
    sorter.Sort();

    std::vector<std::string> result = ReadAllData(output_id);
    std::vector<std::string> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result.size(), expected.size());
    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, true));
}

/**
 * @brief Тест сортировки уже отсортированных строк
 */
TEST_F(StringSortingTestBase, AlreadySortedStrings) {
    const std::string input_id = "sorted_input";
    const std::string output_id = "sorted_output";

    std::vector<std::string> test_data = {"apple", "banana", "cherry", "date", "elderberry", "fig"};

    CreateTestData(input_id, test_data);

    KWayMergeSorter<std::string> sorter(*factory, input_id, output_id, 1024, 2, 10, true);
    sorter.Sort();

    std::vector<std::string> result = ReadAllData(output_id);

    EXPECT_EQ(result, test_data);
    EXPECT_TRUE(IsSorted(result, true));
}

/**
 * @brief Тест сортировки строк в обратном порядке
 */
TEST_F(StringSortingTestBase, ReverseSortedStrings) {
    const std::string input_id = "reverse_input";
    const std::string output_id = "reverse_output";

    std::vector<std::string> test_data = {
      "zebra", "yellow", "watermelon", "violet", "umbrella", "tiger"};

    CreateTestData(input_id, test_data);

    KWayMergeSorter<std::string> sorter(*factory, input_id, output_id, 1024, 2, 10, true);
    sorter.Sort();

    std::vector<std::string> result = ReadAllData(output_id);
    std::vector<std::string> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, true));
}

/**
 * @brief Тест сортировки одинаковых строк
 */
TEST_F(StringSortingTestBase, DuplicateStrings) {
    const std::string input_id = "duplicate_input";
    const std::string output_id = "duplicate_output";

    std::vector<std::string> test_data = {
      "apple", "banana", "apple", "cherry", "banana", "apple", "date"};

    CreateTestData(input_id, test_data);

    KWayMergeSorter<std::string> sorter(*factory, input_id, output_id, 1024, 2, 10, true);
    sorter.Sort();

    std::vector<std::string> result = ReadAllData(output_id);
    std::vector<std::string> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, true));
}
