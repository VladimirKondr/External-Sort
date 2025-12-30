/**
 * @file test_file_stream.cpp
 * @brief Tests for file streams and their factories
 */

#include "file_stream.hpp"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

struct ComplexType {
    uint32_t id;
    std::string name;

    bool Serialize(FILE* file) const {
        if (fwrite(&id, sizeof(id), 1, file) != 1) {
            return false;
        }
        serialization::Serializer<std::string> str_serializer;
        return str_serializer.Serialize(name, file);
    }

    // Deserialize is not needed for this test, but it can be added for completeness
    bool Deserialize(FILE* file) {
        if (fread(&id, sizeof(id), 1, file) != 1) {
            return false;
        }
        serialization::Serializer<std::string> str_serializer;
        return str_serializer.Deserialize(name, file);
    }

    // Important for verification: add GetSerializedSize
    uint64_t GetSerializedSize() const {
        return sizeof(id) + sizeof(uint64_t) + name.length();
    }
};

/**
 * @brief Fixture for file stream tests
 */
class FileStreamTest : public ::testing::Test {
protected:
    void SetUp() override {
        logging::SetDefaultLogger();

        test_dir_ = "test_file_streams";
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
        std::filesystem::create_directory(test_dir_);

        test_file_ = (std::filesystem::path(test_dir_) / "test_file.bin").string();
        factory_ = std::make_unique<io::FileStreamFactory<int>>(test_dir_);
    }

    void TearDown() override {
        factory_.reset();
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
    }

    std::string test_dir_;
    std::string test_file_;
    std::unique_ptr<io::FileStreamFactory<int>> factory_;

    void CreateTestFile(const std::string& filename, const std::vector<int>& data) {
        auto output = factory_->CreateOutputStream(filename, 100);
        for (int value : data) {
            output->Write(value);
        }
        output->Finalize();
    }
};

/**
 * @brief Test creating and writing to file stream
 */
TEST_F(FileStreamTest, OutputStreamWriteAndFinalize) {
    const std::vector<int> test_data = {1, 2, 3, 4, 5};

    {
        auto output = factory_->CreateOutputStream(test_file_, 100);

        EXPECT_EQ(output->GetTotalElementsWritten(), 0);
        EXPECT_EQ(output->GetId(), test_file_);

        for (uint64_t i = 0; i < test_data.size(); ++i) {
            output->Write(test_data[i]);
            EXPECT_EQ(output->GetTotalElementsWritten(), i + 1);
        }

        output->Finalize();
        EXPECT_EQ(output->GetTotalElementsWritten(), test_data.size());
    }

    EXPECT_TRUE(std::filesystem::exists(test_file_));
}

/**
 * @brief Test reading from file stream
 */
TEST_F(FileStreamTest, InputStreamRead) {
    const std::vector<int> test_data = {10, 20, 30, 40, 50};
    CreateTestFile(test_file_, test_data);

    auto input = factory_->CreateInputStream(test_file_, 100);

    EXPECT_FALSE(input->IsEmptyOriginalStorage());
    EXPECT_FALSE(input->IsExhausted());

    std::vector<int> read_data;
    while (!input->IsExhausted()) {
        read_data.push_back(input->TakeValue());
        input->Advance();
    }

    EXPECT_EQ(read_data, test_data);
    EXPECT_TRUE(input->IsExhausted());
}

/**
 * @brief Test working with an empty file
 */
TEST_F(FileStreamTest, EmptyFile) {
    CreateTestFile(test_file_, {});

    auto input = factory_->CreateInputStream(test_file_, 100);

    EXPECT_TRUE(input->IsEmptyOriginalStorage());
    EXPECT_TRUE(input->IsExhausted());

    EXPECT_THROW(input->Value(), std::logic_error);  // NOLINT(cppcoreguidelines-avoid-goto)
}

/**
 * @brief Test writing to stream with small buffer
 */
TEST_F(FileStreamTest, SmallBufferWrite) {
    const std::vector<int> test_data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    const std::string filename_str = test_file_;

    {
        auto output = factory_->CreateOutputStream(test_file_, 2);
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
        raw_file_check.read(reinterpret_cast<char*>(&num_elements_in_file_header),
                            sizeof(uint64_t));

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
        auto input = factory_->CreateInputStream(test_file_, 100);
        ASSERT_NE(input, nullptr);

        std::vector<int> read_data_via_stream;
        while (!input->IsExhausted()) {
            read_data_via_stream.push_back(input->TakeValue());
            input->Advance();
        }

        EXPECT_EQ(read_data_via_stream.size(), test_data.size())
            << "Mismatch in number of elements read via FileInputStream.";
        EXPECT_EQ(read_data_via_stream, test_data)
            << "Data mismatch when reading via FileInputStream.";
    }
}

/**
 * @brief Test reading with small buffer
 */
TEST_F(FileStreamTest, SmallBufferRead) {
    const std::vector<int> test_data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    CreateTestFile(test_file_, test_data);

    auto input = factory_->CreateInputStream(test_file_, 2);

    std::vector<int> read_data;
    while (!input->IsExhausted()) {
        read_data.push_back(input->TakeValue());
        input->Advance();
    }

    EXPECT_EQ(read_data, test_data);
}

/**
 * @brief Test file stream factory operations
 */
TEST_F(FileStreamTest, FactoryOperations) {
    io::StorageId temp_id;
    {
        auto temp_output = factory_->CreateTempOutputStream(temp_id, 100);
        EXPECT_FALSE(temp_id.empty());
        EXPECT_TRUE(temp_id.find(test_dir_) != std::string::npos);

        temp_output->Write(42);
        temp_output->Finalize();
    }

    EXPECT_TRUE(factory_->StorageExists(temp_id));

    std::string permanent_id = (std::filesystem::path(test_dir_) / "permanent.bin").string();
    factory_->MakeStoragePermanent(temp_id, permanent_id);

    EXPECT_TRUE(factory_->StorageExists(permanent_id));

    auto input = factory_->CreateInputStream(permanent_id, 100);
    EXPECT_EQ(input->Value(), 42);

    factory_->DeleteStorage(permanent_id);
    EXPECT_FALSE(factory_->StorageExists(permanent_id));
}

/**
 * @brief Test temporary file context
 */
TEST_F(FileStreamTest, TempStorageContext) {
    io::StorageId context = factory_->GetTempStorageContextId();
    EXPECT_FALSE(context.empty());
    EXPECT_TRUE(context.find(test_dir_) != std::string::npos);
}

/**
 * @brief Test multiple writes and finalization
 */
TEST_F(FileStreamTest, MultipleFinalize) {
    auto output = factory_->CreateOutputStream(test_file_, 100);

    output->Write(1);
    output->Write(2);
    output->Finalize();

    EXPECT_NO_THROW(output->Finalize());  // NOLINT(cppcoreguidelines-avoid-goto)

    EXPECT_THROW(output->Write(3), std::logic_error);  // NOLINT(cppcoreguidelines-avoid-goto)
}

/**
 * @brief Test error handling when opening non-existent file
 */
TEST_F(FileStreamTest, NonExistentFileError) {
    std::string non_existent = (std::filesystem::path(test_dir_) / "non_existent.bin").string();

    EXPECT_THROW(  // NOLINT(cppcoreguidelines-avoid-goto)
        { auto input = factory_->CreateInputStream(non_existent, 100); }, std::runtime_error);
}

/**
 * @brief Test large data
 */
TEST_F(FileStreamTest, LargeData) {
    const uint64_t large_size = 10'000;
    std::vector<int> large_data;
    large_data.reserve(large_size);

    for (uint64_t i = 0; i < large_size; ++i) {
        large_data.push_back(static_cast<int>(i));
    }

    CreateTestFile(test_file_, large_data);

    auto input = factory_->CreateInputStream(test_file_, 1000);
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
 * @brief Test working with different data types
 */
TEST(FileStreamGenericTest, DifferentTypes) {
    std::string test_dir = "test_file_types";
    if (std::filesystem::exists(test_dir)) {
        std::filesystem::remove_all(test_dir);
    }
    std::filesystem::create_directory(test_dir);

    {
        io::FileStreamFactory<double> factory(test_dir);
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
        io::FileStreamFactory<uint64_t> factory(test_dir);
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

class FileStreamBytesWrittenTest : public ::testing::Test {
protected:
    void SetUp() override {
        logging::SetDefaultLogger();
        test_dir_ = "test_bytes_written";
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
        std::filesystem::create_directory(test_dir_);
    }

    void TearDown() override {
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
    }

    std::string test_dir_;
};

/**
 * @brief Verifies byte counting for POD types.
 */
TEST_F(FileStreamBytesWrittenTest, BytesWrittenForPodType) {
    std::string test_file = (std::filesystem::path(test_dir_) / "pod_test.bin").string();
    io::FileStreamFactory<int> factory(test_dir_);

    const std::vector<int> test_data = {10, 20, 30, 40, 50};
    uint64_t expected_bytes = sizeof(uint64_t) + test_data.size() * sizeof(int);

    {
        auto output = factory.CreateOutputStream(test_file, 100);

        // Immediately after creation the stream should contain only the header
        EXPECT_EQ(output->GetTotalBytesWritten(), sizeof(uint64_t));

        for (int val : test_data) {
            output->Write(val);
        }
        output->Finalize();

        // Check the internal counter
        EXPECT_EQ(output->GetTotalBytesWritten(), expected_bytes);
    }

    // Verify actual file size on disk
    EXPECT_TRUE(std::filesystem::exists(test_file));
    EXPECT_EQ(std::filesystem::file_size(test_file), expected_bytes);
}

/**
 * @brief Verifies byte counting for variable-size types (std::string).
 */
TEST_F(FileStreamBytesWrittenTest, BytesWrittenForNonPodType) {
    std::string test_file = (std::filesystem::path(test_dir_) / "string_test.bin").string();
    io::FileStreamFactory<std::string> factory(test_dir_);

    const std::vector<std::string> test_data = {"hello", "world", "", "тест 🚀"};

    uint64_t expected_bytes = sizeof(uint64_t);  // File header
    for (const auto& s : test_data) {
        // For each string: sizeof(length) + string data
        expected_bytes += sizeof(uint64_t) + s.length();
    }

    {
        auto output = factory.CreateOutputStream(test_file, 100);
        EXPECT_EQ(output->GetTotalBytesWritten(), sizeof(uint64_t));

        for (const auto& val : test_data) {
            output->Write(val);
        }
        output->Finalize();

        EXPECT_EQ(output->GetTotalBytesWritten(), expected_bytes);
    }

    EXPECT_TRUE(std::filesystem::exists(test_file));
    EXPECT_EQ(std::filesystem::file_size(test_file), expected_bytes);
}

/**
 * @brief Verifies byte counting for complex user-defined type.
 */
TEST_F(FileStreamBytesWrittenTest, BytesWrittenForComplexType) {
    std::string test_file = (std::filesystem::path(test_dir_) / "complex_test.bin").string();
    io::FileStreamFactory<ComplexType> factory(test_dir_);

    const std::vector<ComplexType> test_data = {{1, "first"}, {2, "second long name"}};

    uint64_t expected_bytes = sizeof(uint64_t);  // File header
    for (const auto& item : test_data) {
        expected_bytes += item.GetSerializedSize();
    }

    {
        auto output = factory.CreateOutputStream(test_file, 100);
        EXPECT_EQ(output->GetTotalBytesWritten(), sizeof(uint64_t));

        for (const auto& val : test_data) {
            output->Write(val);
        }
        output->Finalize();

        EXPECT_EQ(output->GetTotalBytesWritten(), expected_bytes);
    }

    EXPECT_TRUE(std::filesystem::exists(test_file));
    EXPECT_EQ(std::filesystem::file_size(test_file), expected_bytes);
}

/**
 * @brief Verifies that an empty file has the header size.
 */
TEST_F(FileStreamBytesWrittenTest, BytesWrittenForEmptyFile) {
    std::string test_file = (std::filesystem::path(test_dir_) / "empty_test.bin").string();
    io::FileStreamFactory<int> factory(test_dir_);

    uint64_t expected_bytes = sizeof(uint64_t);

    {
        auto output = factory.CreateOutputStream(test_file, 100);
        EXPECT_EQ(output->GetTotalBytesWritten(), expected_bytes);
        output->Finalize();
        EXPECT_EQ(output->GetTotalBytesWritten(), expected_bytes);
    }

    EXPECT_TRUE(std::filesystem::exists(test_file));
    EXPECT_EQ(std::filesystem::file_size(test_file), expected_bytes);
}

/**
 * @brief Verifies correctness of byte counting during automatic buffer flushing.
 */
TEST_F(FileStreamBytesWrittenTest, BytesWrittenWithBufferFlushing) {
    std::string test_file = (std::filesystem::path(test_dir_) / "flush_test.bin").string();
    io::FileStreamFactory<std::string> factory(test_dir_);

    // Use a small buffer to trigger flushing
    const uint64_t buffer_size = 2;
    auto output = factory.CreateOutputStream(test_file, buffer_size);

    uint64_t running_byte_count = sizeof(uint64_t);
    EXPECT_EQ(output->GetTotalBytesWritten(), running_byte_count);

    // 1. Write the first element. It stays in the buffer.
    std::string s1 = "one";
    output->Write(s1);
    // The byte counter should NOT change since no flush has occurred yet
    EXPECT_EQ(output->GetTotalBytesWritten(), running_byte_count);

    // 2. Write the second element. The buffer fills and is flushed.
    std::string s2 = "two";
    output->Write(s2);
    running_byte_count += (sizeof(uint64_t) + s1.length());
    running_byte_count += (sizeof(uint64_t) + s2.length());
    // Now the counter should be updated
    EXPECT_EQ(output->GetTotalBytesWritten(), running_byte_count);

    // 3. Write the third element. It stays in the buffer.
    std::string s3 = "three";
    output->Write(s3);
    EXPECT_EQ(output->GetTotalBytesWritten(), running_byte_count);

    // 4. Finalize. The remaining element is flushed.
    output->Finalize();
    running_byte_count += (sizeof(uint64_t) + s3.length());
    EXPECT_EQ(output->GetTotalBytesWritten(), running_byte_count);

    // Final check of file size
    EXPECT_TRUE(std::filesystem::exists(test_file));
    EXPECT_EQ(std::filesystem::file_size(test_file), running_byte_count);
}
