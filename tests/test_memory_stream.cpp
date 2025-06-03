/**
 * @file test_memory_stream.cpp
 * @brief Тесты для in-memory потоков и их фабрики
 * @author External Sort Library
 * @version 1.0
 */

#include <gtest/gtest.h>
#include "memory_stream.hpp"

using namespace external_sort;

/**
 * @brief Фикстура для тестов in-memory потоков
 */
class MemoryStreamTest : public ::testing::Test {
protected:
    void SetUp() override {
        factory = std::make_unique<InMemoryStreamFactory<int>>();
    }
    
    void TearDown() override {
        factory.reset();
    }
    
    std::unique_ptr<InMemoryStreamFactory<int>> factory;
    
    
    void CreateTestData(const std::string& storage_id, const std::vector<int>& data) {
        auto output = factory->CreateOutputStream(storage_id, 100);
        for (int value : data) {
            output->Write(value);
        }
        output->Finalize();
    }
};

/**
 * @brief Тест создания и записи в memory поток
 */
TEST_F(MemoryStreamTest, OutputStreamWriteAndFinalize) {
    const std::string storage_id = "test_output";
    const std::vector<int> test_data = {1, 2, 3, 4, 5};
    
    {
        auto output = factory->CreateOutputStream(storage_id, 100);
        
        
        EXPECT_EQ(output->GetTotalElementsWritten(), 0);
        EXPECT_EQ(output->GetId(), storage_id);
        
        
        for (size_t i = 0; i < test_data.size(); ++i) {
            output->Write(test_data[i]);
            EXPECT_EQ(output->GetTotalElementsWritten(), i + 1);
        }
        
        
        output->Finalize();
        EXPECT_EQ(output->GetTotalElementsWritten(), test_data.size());
    }
    
    
    EXPECT_TRUE(factory->StorageExists(storage_id));
    EXPECT_EQ(factory->GetStorageDeclaredSize(storage_id), test_data.size());
    
    auto stored_data = factory->GetStorageData(storage_id);
    ASSERT_NE(stored_data, nullptr);
    EXPECT_EQ(*stored_data, test_data);
}

/**
 * @brief Тест чтения из memory потока
 */
TEST_F(MemoryStreamTest, InputStreamRead) {
    const std::string storage_id = "test_input";
    const std::vector<int> test_data = {10, 20, 30, 40, 50};
    CreateTestData(storage_id, test_data);
    
    auto input = factory->CreateInputStream(storage_id, 100);
    
    
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
 * @brief Тест работы с пустыми данными
 */
TEST_F(MemoryStreamTest, EmptyData) {
    const std::string storage_id = "empty_test";
    CreateTestData(storage_id, {});  
    
    auto input = factory->CreateInputStream(storage_id, 100);
    
    EXPECT_TRUE(input->IsEmptyOriginalStorage());
    EXPECT_TRUE(input->IsExhausted());
    
    
    EXPECT_THROW(input->Value(), std::logic_error);
}

/**
 * @brief Тест фабрики memory потоков
 */
TEST_F(MemoryStreamTest, FactoryOperations) {
    
    StorageId temp_id;
    {
        auto temp_output = factory->CreateTempOutputStream(temp_id, 100);
        EXPECT_FALSE(temp_id.empty());
        EXPECT_TRUE(temp_id.find("temp_") != std::string::npos);
        
        temp_output->Write(42);
        temp_output->Finalize();
    }
    
    EXPECT_TRUE(factory->StorageExists(temp_id));
    EXPECT_EQ(factory->GetStorageDeclaredSize(temp_id), 1);
    
    
    std::string permanent_id = "permanent_storage";
    factory->MakeStoragePermanent(temp_id, permanent_id);
    
    EXPECT_TRUE(factory->StorageExists(permanent_id));
    EXPECT_FALSE(factory->StorageExists(temp_id));  
    
    
    auto input = factory->CreateInputStream(permanent_id, 100);
    EXPECT_EQ(input->Value(), 42);
    
    
    factory->DeleteStorage(permanent_id);
    EXPECT_FALSE(factory->StorageExists(permanent_id));
}

/**
 * @brief Тест контекста временных хранилищ
 */
TEST_F(MemoryStreamTest, TempStorageContext) {
    StorageId context = factory->GetTempStorageContextId();
    EXPECT_FALSE(context.empty());
    EXPECT_EQ(context, "temp_");
}

/**
 * @brief Тест множественной записи и финализации
 */
TEST_F(MemoryStreamTest, MultipleFinalize) {
    const std::string storage_id = "multi_finalize_test";
    auto output = factory->CreateOutputStream(storage_id, 100);
    
    output->Write(1);
    output->Write(2);
    output->Finalize();
    
    
    EXPECT_NO_THROW(output->Finalize());
    
    
    EXPECT_THROW(output->Write(3), std::logic_error);
}

/**
 * @brief Тест обработки ошибок при чтении несуществующего хранилища
 */
TEST_F(MemoryStreamTest, NonExistentStorageError) {
    EXPECT_THROW({
        auto input = factory->CreateInputStream("non_existent", 100);
    }, std::runtime_error);
}

/**
 * @brief Тест больших данных
 */
TEST_F(MemoryStreamTest, LargeData) {
    const std::string storage_id = "large_data_test";
    const size_t large_size = 10000;
    std::vector<int> large_data;
    large_data.reserve(large_size);
    
    for (size_t i = 0; i < large_size; ++i) {
        large_data.push_back(static_cast<int>(i));
    }
    
    CreateTestData(storage_id, large_data);
    
    auto input = factory->CreateInputStream(storage_id, 1000);
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
 * @brief Тест множественных потоков к одному хранилищу
 */
TEST_F(MemoryStreamTest, MultipleStreamsToSameStorage) {
    const std::string storage_id = "shared_storage";
    const std::vector<int> test_data = {1, 2, 3, 4, 5};
    CreateTestData(storage_id, test_data);
    
    
    auto input1 = factory->CreateInputStream(storage_id, 100);
    auto input2 = factory->CreateInputStream(storage_id, 100);
    
    
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
 * @brief Тест переименования хранилища (self-assignment)
 */
TEST_F(MemoryStreamTest, SelfRename) {
    const std::string storage_id = "self_rename_test";
    const std::vector<int> test_data = {1, 2, 3};
    CreateTestData(storage_id, test_data);
    
    
    EXPECT_NO_THROW(factory->MakeStoragePermanent(storage_id, storage_id));
    EXPECT_TRUE(factory->StorageExists(storage_id));
    
    auto stored_data = factory->GetStorageData(storage_id);
    ASSERT_NE(stored_data, nullptr);
    EXPECT_EQ(*stored_data, test_data);
}

/**
 * @brief Тест ошибки при попытке сделать постоянным несуществующее временное хранилище
 */
TEST_F(MemoryStreamTest, MakePermanentNonExistentTemp) {
    EXPECT_THROW({
        factory->MakeStoragePermanent("non_existent_temp", "permanent");
    }, std::runtime_error);
}

/**
 * @brief Тест уникальности временных ID
 */
TEST_F(MemoryStreamTest, UniqueTempIds) {
    std::set<StorageId> temp_ids;
    
    
    for (int i = 0; i < 10; ++i) {
        StorageId temp_id;
        auto temp_output = factory->CreateTempOutputStream(temp_id, 100);
        
        EXPECT_TRUE(temp_ids.find(temp_id) == temp_ids.end()) << "Duplicate temp ID: " << temp_id;
        temp_ids.insert(temp_id);
        
        temp_output->Write(i);
        temp_output->Finalize();
    }
    
    EXPECT_EQ(temp_ids.size(), 10);
}

/**
 * @brief Тест работы с различными типами данных
 */
TEST(MemoryStreamGenericTest, DifferentTypes) {
    
    {
        InMemoryStreamFactory<double> factory;
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
        for (size_t i = 0; i < test_data.size(); ++i) {
            EXPECT_DOUBLE_EQ(read_data[i], test_data[i]);
        }
    }
    
    
    {
        InMemoryStreamFactory<std::string> factory;
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
        
        InMemoryStreamFactory<TestStruct> factory;
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
