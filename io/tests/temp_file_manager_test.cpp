/**
 * @file test_temp_file_manager.cpp
 * @brief Tests for TempFileManager class
 */

#include "temp_file_manager.hpp"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

/**
 * @brief Fixture for TempFileManager tests
 */
class TempFileManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        logging::SetDefaultLogger();

        test_dir_name_ = "test_temp_manager_dir";

        if (std::filesystem::exists(test_dir_name_)) {
            std::filesystem::remove_all(test_dir_name_);
        }
    }

    void TearDown() override {
        if (std::filesystem::exists(test_dir_name_)) {
            std::filesystem::remove_all(test_dir_name_);
        }
    }

    std::string test_dir_name_;
};

/**
 * @brief Test manager creation and base directory
 */
TEST_F(TempFileManagerTest, ConstructorCreatesDirectory) {
    EXPECT_FALSE(std::filesystem::exists(test_dir_name_));

    {
        io::TempFileManager manager(test_dir_name_);

        EXPECT_TRUE(std::filesystem::exists(test_dir_name_));
        EXPECT_TRUE(std::filesystem::is_directory(test_dir_name_));

        auto expected_path = std::filesystem::current_path() / test_dir_name_;
        EXPECT_EQ(manager.GetBaseDirPath(), expected_path);
    }

    EXPECT_FALSE(std::filesystem::exists(test_dir_name_));
}

/**
 * @brief Test working with existing directory
 */
TEST_F(TempFileManagerTest, ExistingDirectory) {
    std::filesystem::create_directory(test_dir_name_);
    auto existing_file = std::filesystem::path(test_dir_name_) / "existing_file.txt";
    std::ofstream(existing_file) << "test content";

    EXPECT_TRUE(std::filesystem::exists(test_dir_name_));
    EXPECT_TRUE(std::filesystem::exists(existing_file));

    {
        io::TempFileManager manager(test_dir_name_);

        EXPECT_TRUE(std::filesystem::exists(test_dir_name_));
        EXPECT_TRUE(std::filesystem::exists(existing_file));
    }

    EXPECT_TRUE(std::filesystem::exists(test_dir_name_));
    EXPECT_TRUE(std::filesystem::exists(existing_file));
}

/**
 * @brief Test cleanup of non-existent files
 */
TEST_F(TempFileManagerTest, CleanupNonExistentFile) {
    io::TempFileManager manager(test_dir_name_);

    std::string non_existent = (manager.GetBaseDirPath() / "non_existent.txt").string();

    EXPECT_NO_THROW(manager.CleanupFile(non_existent));  // NOLINT(cppcoreguidelines-avoid-goto)
}

/**
 * @brief Test working with multiple managers
 */
TEST_F(TempFileManagerTest, MultipleManagers) {
    std::string dir1 = test_dir_name_ + "_1";
    std::string dir2 = test_dir_name_ + "_2";

    {
        io::TempFileManager manager1(dir1);
        io::TempFileManager manager2(dir2);

        EXPECT_TRUE(std::filesystem::exists(dir1));
        EXPECT_TRUE(std::filesystem::exists(dir2));

        std::string file1 = manager1.GenerateTempFilename();
        std::string file2 = manager2.GenerateTempFilename();

        EXPECT_TRUE(file1.find(dir1) != std::string::npos);
        EXPECT_TRUE(file2.find(dir2) != std::string::npos);

        std::ofstream(file1) << "content1";
        std::ofstream(file2) << "content2";

        EXPECT_TRUE(std::filesystem::exists(file1));
        EXPECT_TRUE(std::filesystem::exists(file2));
    }

    EXPECT_FALSE(std::filesystem::exists(dir1));
    EXPECT_FALSE(std::filesystem::exists(dir2));
}

/**
 * @brief Test move semantics
 */
TEST_F(TempFileManagerTest, MoveSemantics) {
    std::string file_path;

    {
        io::TempFileManager manager1(test_dir_name_);
        file_path = manager1.GenerateTempFilename();
        std::ofstream(file_path) << "test";

        io::TempFileManager manager2 = std::move(manager1);

        EXPECT_TRUE(std::filesystem::exists(test_dir_name_));
        EXPECT_TRUE(std::filesystem::exists(file_path));

        EXPECT_EQ(manager2.GetBaseDirPath(), std::filesystem::current_path() / test_dir_name_);
    }

    EXPECT_FALSE(std::filesystem::exists(test_dir_name_));
    EXPECT_FALSE(std::filesystem::exists(file_path));
}

/**
 * @brief Test directory creation error handling
 */
TEST_F(TempFileManagerTest, DirectoryCreationError) {
    std::string invalid_dir_name = "/root/impossible_to_create_directory_here_12345";

    if (std::filesystem::exists("/root") &&
        std::filesystem::status("/root").permissions() != std::filesystem::perms::none) {
        GTEST_SKIP() << "Cannot test directory creation failure on this system";
    }

    EXPECT_THROW(  // NOLINT(cppcoreguidelines-avoid-goto)
        { io::TempFileManager manager(invalid_dir_name); }, std::runtime_error);
}
