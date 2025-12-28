/**
 * @file test_memory_stream.cpp
 * @brief Tests for in-memory streams and their factory
 * @author External Sort Library
 * @version 1.0
 */

#include "memory_stream.hpp"

#include <gtest/gtest.h>
#include "Registry.hpp"

/**
 * @brief Fixture for in-memory stream tests
 */
class MemoryStreamTest : public ::testing::Test {
protected:
    void SetUp() override {
        logging::SetDefaultLogger();

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
};

/**
 * @brief Test creating and writing to memory stream
 */
TEST_F(MemoryStreamTest, OutputStreamWriteAndFinalize) {
    const std::string storage_id = "test_output";
    const std::vector<int> test_data = {1, 2, 3, 4, 5};

    {
        auto output = factory_->CreateOutputStream(storage_id, 100);

        EXPECT_EQ(output->GetTotalElementsWritten(), 0);
        EXPECT_EQ(output->GetId(), storage_id);

        for (uint64_t i = 0; i < test_data.size(); ++i) {
            output->Write(test_data[i]);
            EXPECT_EQ(output->GetTotalElementsWritten(), i + 1);
        }

        output->Finalize();
        EXPECT_EQ(output->GetTotalElementsWritten(), test_data.size());
    }

    EXPECT_TRUE(factory_->StorageExists(storage_id));
    EXPECT_EQ(factory_->GetStorageDeclaredSize(storage_id), test_data.size());

    auto stored_data = factory_->GetStorageData(storage_id);
    ASSERT_NE(stored_data, nullptr);
    EXPECT_EQ(*stored_data, test_data);
}

/**
 * @brief Test reading from memory stream
 */
TEST_F(MemoryStreamTest, InputStreamRead) {
    const std::string storage_id = "test_input";
    const std::vector<int> test_data = {10, 20, 30, 40, 50};
    CreateTestData(storage_id, test_data);

    auto input = factory_->CreateInputStream(storage_id, 100);

    EXPECT_FALSE(input->IsEmptyOriginalStorage());
    EXPECT_FALSE(input->IsExhausted());

    std::vector<int> read_data;
    while (!input->IsExhausted()) {
        read_data.push_back(input->Value());
        input->Advance();
    }

    EXPECT_EQ(read_data, test_data);
    EXPECT_TRUE(input->IsExhausted());
}

/**
 * @brief Test working with empty data
 */
TEST_F(MemoryStreamTest, EmptyData) {
    const std::string storage_id = "empty_test";
    CreateTestData(storage_id, {});

    auto input = factory_->CreateInputStream(storage_id, 100);

    EXPECT_TRUE(input->IsEmptyOriginalStorage());
    EXPECT_TRUE(input->IsExhausted());

    EXPECT_THROW(input->Value(), std::logic_error);  // NOLINT(cppcoreguidelines-avoid-goto)
}

/**
 * @brief Test memory stream factory operations
 */
TEST_F(MemoryStreamTest, FactoryOperations) {
    io::StorageId temp_id;
    {
        auto temp_output = factory_->CreateTempOutputStream(temp_id, 100);
        EXPECT_FALSE(temp_id.empty());
        EXPECT_TRUE(temp_id.find("temp_") != std::string::npos);

        temp_output->Write(42);
        temp_output->Finalize();
    }

    EXPECT_TRUE(factory_->StorageExists(temp_id));
    EXPECT_EQ(factory_->GetStorageDeclaredSize(temp_id), 1);

    std::string permanent_id = "permanent_storage";
    factory_->MakeStoragePermanent(temp_id, permanent_id);

    EXPECT_TRUE(factory_->StorageExists(permanent_id));
    EXPECT_FALSE(factory_->StorageExists(temp_id));

    auto input = factory_->CreateInputStream(permanent_id, 100);
    EXPECT_EQ(input->Value(), 42);

    factory_->DeleteStorage(permanent_id);
    EXPECT_FALSE(factory_->StorageExists(permanent_id));
}

/**
 * @brief Test multiple writes and finalization
 */
TEST_F(MemoryStreamTest, MultipleFinalize) {
    const std::string storage_id = "multi_finalize_test";
    auto output = factory_->CreateOutputStream(storage_id, 100);

    output->Write(1);
    output->Write(2);
    output->Finalize();

    EXPECT_NO_THROW(output->Finalize());  // NOLINT(cppcoreguidelines-avoid-goto)

    EXPECT_THROW(output->Write(3), std::logic_error);  // NOLINT(cppcoreguidelines-avoid-goto)
}

/**
 * @brief Test error handling when reading non-existent storage
 */
TEST_F(MemoryStreamTest, NonExistentStorageError) {
    EXPECT_THROW(  // NOLINT(cppcoreguidelines-avoid-goto)
        { auto input = factory_->CreateInputStream("non_existent", 100); }, std::runtime_error);
}

/**
 * @brief Test large data
 */
TEST_F(MemoryStreamTest, LargeData) {
    const std::string storage_id = "large_data_test";
    const uint64_t large_size = 10'000;
    std::vector<int> large_data;
    large_data.reserve(large_size);

    for (uint64_t i = 0; i < large_size; ++i) {
        large_data.push_back(static_cast<int>(i));
    }

    CreateTestData(storage_id, large_data);

    auto input = factory_->CreateInputStream(storage_id, 1000);
    std::vector<int> read_data;
    read_data.reserve(large_size);

    while (!input->IsExhausted()) {
        read_data.push_back(input->Value());
        input->Advance();
    }

    EXPECT_EQ(read_data.size(), large_size);
    EXPECT_EQ(read_data, large_data);
}

/**
 * @brief Test multiple streams to the same storage
 */
TEST_F(MemoryStreamTest, MultipleStreamsToSameStorage) {
    const std::string storage_id = "shared_storage";
    const std::vector<int> test_data = {1, 2, 3, 4, 5};
    CreateTestData(storage_id, test_data);

    auto input1 = factory_->CreateInputStream(storage_id, 100);
    auto input2 = factory_->CreateInputStream(storage_id, 100);

    std::vector<int> read_data1;
    while (!input1->IsExhausted()) {
        read_data1.push_back(input1->Value());
        input1->Advance();
    }

    std::vector<int> read_data2;
    while (!input2->IsExhausted()) {
        read_data2.push_back(input2->Value());
        input2->Advance();
    }

    EXPECT_EQ(read_data1, test_data);
    EXPECT_EQ(read_data2, test_data);
}

/**
 * @brief Test storage rename (self-assignment)
 */
TEST_F(MemoryStreamTest, SelfRename) {
    const std::string storage_id = "self_rename_test";
    const std::vector<int> test_data = {1, 2, 3};
    CreateTestData(storage_id, test_data);

    EXPECT_NO_THROW(factory_->MakeStoragePermanent(  // NOLINT(cppcoreguidelines-avoid-goto)
        storage_id, storage_id));
    EXPECT_TRUE(factory_->StorageExists(storage_id));

    auto stored_data = factory_->GetStorageData(storage_id);
    ASSERT_NE(stored_data, nullptr);
    EXPECT_EQ(*stored_data, test_data);
}

/**
 * @brief Test error when making non-existent temp storage permanent
 */
TEST_F(MemoryStreamTest, MakePermanentNonExistentTemp) {
    EXPECT_THROW(  // NOLINT(cppcoreguidelines-avoid-goto)
        { factory_->MakeStoragePermanent("non_existent_temp", "permanent"); }, std::runtime_error);
}

/**
 * @brief Test uniqueness of temporary IDs
 */
TEST_F(MemoryStreamTest, UniqueTempIds) {
    std::set<io::StorageId> temp_ids;

    for (int i = 0; i < 10; ++i) {
        io::StorageId temp_id;
        auto temp_output = factory_->CreateTempOutputStream(temp_id, 100);

        EXPECT_TRUE(temp_ids.find(temp_id) == temp_ids.end()) << "Duplicate temp ID: " << temp_id;
        temp_ids.insert(temp_id);

        temp_output->Write(i);
        temp_output->Finalize();
    }

    EXPECT_EQ(temp_ids.size(), 10);
}

/**
 * @brief Test working with different data types
 */
TEST(MemoryStreamGenericTest, DifferentTypes) {
    {
        io::InMemoryStreamFactory<double> factory;
        std::string storage_id = "double_test";

        std::vector<double> test_data = {3.14, 2.71, 1.41, 1.73};

        {
            auto output = factory.CreateOutputStream(storage_id, 100);
            for (double value : test_data) {
                output->Write(value);
            }
            output->Finalize();
        }

        auto input = factory.CreateInputStream(storage_id, 100);
        std::vector<double> read_data;
        while (!input->IsExhausted()) {
            read_data.push_back(input->Value());
            input->Advance();
        }

        EXPECT_EQ(read_data.size(), test_data.size());
        for (uint64_t i = 0; i < test_data.size(); ++i) {
            EXPECT_DOUBLE_EQ(read_data[i], test_data[i]);
        }
    }

    {
        io::InMemoryStreamFactory<std::string> factory;
        std::string storage_id = "string_test";

        std::vector<std::string> test_data = {"hello", "world", "test", "data"};

        {
            auto output = factory.CreateOutputStream(storage_id, 100);
            for (const std::string& value : test_data) {
                output->Write(value);
            }
            output->Finalize();
        }

        auto input = factory.CreateInputStream(storage_id, 100);
        std::vector<std::string> read_data;
        while (!input->IsExhausted()) {
            read_data.push_back(input->Value());
            input->Advance();
        }

        EXPECT_EQ(read_data, test_data);
    }

    {
        struct TestStruct {
            int x, y;

            bool operator==(const TestStruct& other) const {
                return x == other.x && y == other.y;
            }
        };

        io::InMemoryStreamFactory<TestStruct> factory;
        std::string storage_id = "struct_test";

        std::vector<TestStruct> test_data = {{1, 2}, {3, 4}, {5, 6}};

        {
            auto output = factory.CreateOutputStream(storage_id, 100);
            for (const TestStruct& value : test_data) {
                output->Write(value);
            }
            output->Finalize();
        }

        auto input = factory.CreateInputStream(storage_id, 100);
        std::vector<TestStruct> read_data;
        while (!input->IsExhausted()) {
            read_data.push_back(input->Value());
            input->Advance();
        }

        EXPECT_EQ(read_data, test_data);
    }
}
