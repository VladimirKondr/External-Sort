/**
 * @file complex_types_test.cpp
 * @brief Tests for sorting complex user-defined types
 */

#include "k_way_merge_sorter.hpp"
#include "file_stream.hpp"

#include <algorithm>
#include <filesystem>
#include <gtest/gtest.h>
#include <string>
#include <vector>

#include <string>

/**
 * @brief Complex structure with multiple fields including non-POD types
 */
struct Person {
    std::string name;
    int32_t age;
    double height;
    uint32_t weight;
    std::string address;

    bool operator<(const Person& other) const {
        return age < other.age;
    }

    bool operator>(const Person& other) const {
        return age > other.age;
    }

    bool operator==(const Person& other) const {
        return name == other.name && age == other.age && height == other.height &&
               weight == other.weight && address == other.address;
    }

    /**
     * @brief Serialize the object to a file
     * @param file File pointer to write to
     * @return true if serialization succeeded
     */
    bool Serialize(FILE* file) const {
        // Serialize name
        size_t name_size = name.size();
        if (fwrite(&name_size, sizeof(size_t), 1, file) != 1) {
            return false;
        }
        if (fwrite(name.data(), sizeof(char), name_size, file) != name_size) {
            return false;
        }

        // Serialize POD fields
        if (fwrite(&age, sizeof(int32_t), 1, file) != 1) {
            return false;
        }
        if (fwrite(&height, sizeof(double), 1, file) != 1) {
            return false;
        }
        if (fwrite(&weight, sizeof(uint32_t), 1, file) != 1) {
            return false;
        }

        // Serialize address
        size_t address_size = address.size();
        if (fwrite(&address_size, sizeof(size_t), 1, file) != 1) {
            return false;
        }
        if (fwrite(address.data(), sizeof(char), address_size, file) != address_size) {
            return false;
        }

        return true;
    }

    /**
     * @brief Deserialize the object from a file
     * @param file File pointer to read from
     * @return true if deserialization succeeded
     */
    bool Deserialize(FILE* file) {
        // Deserialize name
        size_t name_size;
        if (fread(&name_size, sizeof(size_t), 1, file) != 1) {
            return false;
        }
        name.resize(name_size);
        if (fread(name.data(), sizeof(char), name_size, file) != name_size) {
            return false;
        }

        // Deserialize POD fields
        if (fread(&age, sizeof(int32_t), 1, file) != 1) {
            return false;
        }
        if (fread(&height, sizeof(double), 1, file) != 1) {
            return false;
        }
        if (fread(&weight, sizeof(uint32_t), 1, file) != 1) {
            return false;
        }

        // Deserialize address
        size_t address_size;
        if (fread(&address_size, sizeof(size_t), 1, file) != 1) {
            return false;
        }
        address.resize(address_size);
        if (fread(address.data(), sizeof(char), address_size, file) != address_size) {
            return false;
        }

        return true;
    }
};

/**
 * @brief Testing sorting of complex structure
 */
class ComplexTypesTest : public ::testing::Test {
protected:
    std::unique_ptr<io::FileStreamFactory<Person>> factory_;
    std::string test_dir_;

    void SetUp() override {
        test_dir_ = "complex_types_test";

        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
        std::filesystem::create_directory(test_dir_);

        factory_ = std::make_unique<io::FileStreamFactory<Person>>(test_dir_);
    }

    void TearDown() override {
        factory_.reset();
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
    }

    void CreateTestData(const std::string& storage_id, const std::vector<Person>& data) {
        auto output = factory_->CreateOutputStream(storage_id, 100);
        for (const auto& item : data) {
            output->Write(item);
        }
        output->Finalize();
    }

    std::vector<Person> ReadAllData(const std::string& storage_id) {
        auto input = factory_->CreateInputStream(storage_id, 100);
        std::vector<Person> result;
        while (!input->IsExhausted()) {
            result.push_back(input->Value());
            input->Advance();
        }
        return result;
    }

    bool IsSorted(const std::vector<Person>& data, bool ascending = true) {
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
 * @brief Test basic sorting of complex structure
 */
TEST_F(ComplexTypesTest, BasicComplexTypeSorting) {
    const std::string input_id = "input";
    const std::string output_id = "output";

    std::vector<Person> test_data = {{"Alice", 30, 180.5, 75, "123 Main St"},
                                     {"Bob", 25, 170.0, 65, "456 Oak Ave"},
                                     {"Charlie", 35, 175.5, 80, "789 Pine Rd"},
                                     {"David", 20, 165.0, 60, "101 Elm St"},
                                     {"Eve", 28, 172.5, 70, "202 Maple Ln"}};

    CreateTestData(input_id, test_data);

    external_sort::KWayMergeSorter<Person> sorter(*factory_, input_id, output_id,
                                                  sizeof(Person) * 3, 2, 10, true);
    sorter.Sort();

    std::vector<Person> result = ReadAllData(output_id);
    std::vector<Person> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, true));
}

/**
 * @brief Test descending sort
 */
TEST_F(ComplexTypesTest, DescendingSortComplexType) {
    const std::string input_id = "desc_input";
    const std::string output_id = "desc_output";

    std::vector<Person> test_data = {{"David", 20, 165.0, 60, "101 Elm St"},
                                     {"Bob", 25, 170.0, 65, "456 Oak Ave"},
                                     {"Alice", 30, 180.5, 75, "123 Main St"},
                                     {"Charlie", 35, 175.5, 80, "789 Pine Rd"}};

    CreateTestData(input_id, test_data);

    external_sort::KWayMergeSorter<Person> sorter(*factory_, input_id, output_id,
                                                  sizeof(Person) * 2, 2, 10, false);
    sorter.Sort();

    std::vector<Person> result = ReadAllData(output_id);
    std::vector<Person> expected = test_data;
    std::sort(expected.begin(), expected.end(), std::greater<Person>());

    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, false));
}

/**
 * @brief Test sorting with duplicates
 */
TEST_F(ComplexTypesTest, DuplicateValues) {
    const std::string input_id = "dup_input";
    const std::string output_id = "dup_output";

    std::vector<Person> test_data = {
        {"Jake", 30, 180.5, 75, "123 Main St"},
        {"John", 25, 170.0, 65, "456 Oak Ave"},
        {"Mike", 30, 175.5, 80, "789 Pine Rd"},  // duplicate by age
        {"Nick", 25, 165.0, 60, "101 Elm St"},   // duplicate by age
        {"Paul", 30, 172.5, 70, "202 Maple Ln"}  // duplicate by age
    };

    CreateTestData(input_id, test_data);

    external_sort::KWayMergeSorter<Person> sorter(*factory_, input_id, output_id,
                                                  sizeof(Person) * 2, 2, 10, true);
    sorter.Sort();

    std::vector<Person> result = ReadAllData(output_id);
    std::vector<Person> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result.size(), expected.size());
    EXPECT_TRUE(IsSorted(result, true));
}
