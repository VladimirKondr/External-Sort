/**
 * @file test_file_stream.cpp
 * @brief Тесты для файловых потоков и их фабрики
 * @author External Sort Library
 * @version 1.0
 */

#include "file_stream.hpp"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

using namespace external_sort;

/**
 * @brief Фикстура для тестов файловых потоков
 */
class FileStreamTest : public ::testing::Test {
   protected:
    void SetUp() override {
        test_dir = "test_file_streams";
        if (std::filesystem::exists(test_dir)) {
            std::filesystem::remove_all(test_dir);
        }
        std::filesystem::create_directory(test_dir);

        test_file = (std::filesystem::path(test_dir) / "test_file.bin").string();
        factory = std::make_unique<FileStreamFactory<int>>(test_dir);
    }

    void TearDown() override {
        factory.reset();
        if (std::filesystem::exists(test_dir)) {
            std::filesystem::remove_all(test_dir);
        }
    }

    std::string test_dir;
    std::string test_file;
    std::unique_ptr<FileStreamFactory<int>> factory;

    void CreateTestFile(const std::string& filename, const std::vector<int>& data) {
        auto output = factory->CreateOutputStream(filename, 100);
        for (int value : data) {
            output->Write(value);
        }
        output->Finalize();
    }
};

/**
 * @brief Тест создания и записи в файловый поток
 */
TEST_F(FileStreamTest, OutputStreamWriteAndFinalize) {
    const std::vector<int> test_data = {1, 2, 3, 4, 5};

    {
        auto output = factory->CreateOutputStream(test_file, 100);

        EXPECT_EQ(output->GetTotalElementsWritten(), 0);
        EXPECT_EQ(output->GetId(), test_file);

        for (uint64_t i = 0; i < test_data.size(); ++i) {
            output->Write(test_data[i]);
            EXPECT_EQ(output->GetTotalElementsWritten(), i + 1);
        }

        output->Finalize();
        EXPECT_EQ(output->GetTotalElementsWritten(), test_data.size());
    }

    EXPECT_TRUE(std::filesystem::exists(test_file));
}

/**
 * @brief Тест чтения из файлового потока
 */
TEST_F(FileStreamTest, InputStreamRead) {
    const std::vector<int> test_data = {10, 20, 30, 40, 50};
    CreateTestFile(test_file, test_data);

    auto input = factory->CreateInputStream(test_file, 100);

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
 * @brief Тест работы с пустым файлом
 */
TEST_F(FileStreamTest, EmptyFile) {
    CreateTestFile(test_file, {});

    auto input = factory->CreateInputStream(test_file, 100);

    EXPECT_TRUE(input->IsEmptyOriginalStorage());
    EXPECT_TRUE(input->IsExhausted());

    EXPECT_THROW(input->Value(), std::logic_error);
}

/**
 * @brief Тест записи в поток с малым буфером
 */
TEST_F(FileStreamTest, SmallBufferWrite) {
    const std::vector<int> test_data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    const std::string filename_str = test_file;

    {
        auto output = factory->CreateOutputStream(test_file, 2);
        ASSERT_NE(output, nullptr);

        for (int value : test_data) {
            output->Write(value);
        }
        output->Finalize();
    }

    {
        std::ifstream raw_file_check(filename_str, std::ios::binary);
        ASSERT_TRUE(raw_file_check.is_open())
            << "Failed to open file for raw check: " << filename_str;

        uint64_t num_elements_in_file_header = 0;
        raw_file_check.read(
            reinterpret_cast<char*>(&num_elements_in_file_header), sizeof(uint64_t));

        ASSERT_FALSE(raw_file_check.fail()) << "Failed to read header from raw file.";
        ASSERT_EQ(num_elements_in_file_header, test_data.size()) << "Header mismatch in raw file.";

        std::vector<int> raw_read_data;
        raw_read_data.reserve(test_data.size());
        for (uint64_t i = 0; i < test_data.size(); ++i) {
            int element;
            raw_file_check.read(reinterpret_cast<char*>(&element), sizeof(int));
            if (raw_file_check.fail() || (raw_file_check.eof() && i < test_data.size() - 1)) {
                FAIL() << "Failed to read element " << i << " from raw file. "
                       << "EOF: " << raw_file_check.eof() << " Fail: " << raw_file_check.fail()
                       << " Bad: " << raw_file_check.bad();
            }
            raw_read_data.push_back(element);
        }

        EXPECT_EQ(raw_read_data, test_data) << "Data mismatch during raw file check.";

        char dummy;
        raw_file_check.read(&dummy, 1);
        EXPECT_TRUE(raw_file_check.eof())
            << "File contains more data than expected after raw check.";

        raw_file_check.close();
    }

    {
        auto input = factory->CreateInputStream(test_file, 100);
        ASSERT_NE(input, nullptr);

        std::vector<int> read_data_via_stream;
        while (!input->IsExhausted()) {
            read_data_via_stream.push_back(input->Value());
            input->Advance();
        }

        EXPECT_EQ(read_data_via_stream.size(), test_data.size())
            << "Mismatch in number of elements read via FileInputStream.";
        EXPECT_EQ(read_data_via_stream, test_data)
            << "Data mismatch when reading via FileInputStream.";
    }
}

/**
 * @brief Тест чтения с малым буфером
 */
TEST_F(FileStreamTest, SmallBufferRead) {
    const std::vector<int> test_data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    CreateTestFile(test_file, test_data);

    auto input = factory->CreateInputStream(test_file, 2);

    std::vector<int> read_data;
    while (!input->IsExhausted()) {
        read_data.push_back(input->Value());
        input->Advance();
    }

    EXPECT_EQ(read_data, test_data);
}

/**
 * @brief Тест фабрики файловых потоков
 */
TEST_F(FileStreamTest, FactoryOperations) {
    StorageId temp_id;
    {
        auto temp_output = factory->CreateTempOutputStream(temp_id, 100);
        EXPECT_FALSE(temp_id.empty());
        EXPECT_TRUE(temp_id.find(test_dir) != std::string::npos);

        temp_output->Write(42);
        temp_output->Finalize();
    }

    EXPECT_TRUE(factory->StorageExists(temp_id));

    std::string permanent_id = (std::filesystem::path(test_dir) / "permanent.bin").string();
    factory->MakeStoragePermanent(temp_id, permanent_id);

    EXPECT_TRUE(factory->StorageExists(permanent_id));

    auto input = factory->CreateInputStream(permanent_id, 100);
    EXPECT_EQ(input->Value(), 42);

    factory->DeleteStorage(permanent_id);
    EXPECT_FALSE(factory->StorageExists(permanent_id));
}

/**
 * @brief Тест контекста временных файлов
 */
TEST_F(FileStreamTest, TempStorageContext) {
    StorageId context = factory->GetTempStorageContextId();
    EXPECT_FALSE(context.empty());
    EXPECT_TRUE(context.find(test_dir) != std::string::npos);
}

/**
 * @brief Тест множественной записи и финализации
 */
TEST_F(FileStreamTest, MultipleFinalize) {
    auto output = factory->CreateOutputStream(test_file, 100);

    output->Write(1);
    output->Write(2);
    output->Finalize();

    EXPECT_NO_THROW(output->Finalize());

    EXPECT_THROW(output->Write(3), std::logic_error);
}

/**
 * @brief Тест обработки ошибок при открытии несуществующего файла
 */
TEST_F(FileStreamTest, NonExistentFileError) {
    std::string non_existent = (std::filesystem::path(test_dir) / "non_existent.bin").string();

    EXPECT_THROW(
        { auto input = factory->CreateInputStream(non_existent, 100); }, std::runtime_error);
}

/**
 * @brief Тест больших данных
 */
TEST_F(FileStreamTest, LargeData) {
    const uint64_t large_size = 10'000;
    std::vector<int> large_data;
    large_data.reserve(large_size);

    for (uint64_t i = 0; i < large_size; ++i) {
        large_data.push_back(static_cast<int>(i));
    }

    CreateTestFile(test_file, large_data);

    auto input = factory->CreateInputStream(test_file, 1000);
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
 * @brief Тест работы с различными типами данных
 */
TEST(FileStreamGenericTest, DifferentTypes) {
    std::string test_dir = "test_file_types";
    if (std::filesystem::exists(test_dir)) {
        std::filesystem::remove_all(test_dir);
    }
    std::filesystem::create_directory(test_dir);

    {
        FileStreamFactory<double> factory(test_dir);
        std::string file_path = (std::filesystem::path(test_dir) / "double_test.bin").string();

        std::vector<double> test_data = {3.14, 2.71, 1.41, 1.73};

        {
            auto output = factory.CreateOutputStream(file_path, 100);
            for (double value : test_data) {
                output->Write(value);
            }
            output->Finalize();
        }

        auto input = factory.CreateInputStream(file_path, 100);
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
        FileStreamFactory<uint64_t> factory(test_dir);
        std::string file_path = (std::filesystem::path(test_dir) / "uint64_test.bin").string();

        std::vector<uint64_t> test_data = {0, 1'000'000'000ULL, 18'446'744'073'709'551'615ULL};

        {
            auto output = factory.CreateOutputStream(file_path, 100);
            for (uint64_t value : test_data) {
                output->Write(value);
            }
            output->Finalize();
        }

        auto input = factory.CreateInputStream(file_path, 100);
        std::vector<uint64_t> read_data;
        while (!input->IsExhausted()) {
            read_data.push_back(input->Value());
            input->Advance();
        }

        EXPECT_EQ(read_data, test_data);
    }

    std::filesystem::remove_all(test_dir);
}
