/**
 * @file test_temp_file_manager.cpp
 * @brief Тесты для класса TempFileManager
 * @author External Sort Library
 * @version 1.0
 */

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include "temp_file_manager.hpp"

using namespace external_sort;

/**
 * @brief Фикстура для тестов TempFileManager
 */
class TempFileManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_name = "test_temp_manager_dir";
        
        if (std::filesystem::exists(test_dir_name)) {
            std::filesystem::remove_all(test_dir_name);
        }
    }
    
    void TearDown() override {
        
        if (std::filesystem::exists(test_dir_name)) {
            std::filesystem::remove_all(test_dir_name);
        }
    }
    
    std::string test_dir_name;
};

/**
 * @brief Тест создания менеджера и базовой директории
 */
TEST_F(TempFileManagerTest, ConstructorCreatesDirectory) {
    EXPECT_FALSE(std::filesystem::exists(test_dir_name));
    
    {
        TempFileManager manager(test_dir_name);
        
        
        EXPECT_TRUE(std::filesystem::exists(test_dir_name));
        EXPECT_TRUE(std::filesystem::is_directory(test_dir_name));
        
        
        auto expected_path = std::filesystem::current_path() / test_dir_name;
        EXPECT_EQ(manager.GetBaseDirPath(), expected_path);
    }
    
    
    EXPECT_FALSE(std::filesystem::exists(test_dir_name));
}

/**
 * @brief Тест работы с существующей директорией
 */
TEST_F(TempFileManagerTest, ExistingDirectory) {
    
    std::filesystem::create_directory(test_dir_name);
    auto existing_file = std::filesystem::path(test_dir_name) / "existing_file.txt";
    std::ofstream(existing_file) << "test content";
    
    EXPECT_TRUE(std::filesystem::exists(test_dir_name));
    EXPECT_TRUE(std::filesystem::exists(existing_file));
    
    {
        TempFileManager manager(test_dir_name);
        
        
        EXPECT_TRUE(std::filesystem::exists(test_dir_name));
        EXPECT_TRUE(std::filesystem::exists(existing_file));  
    }
    
    
    EXPECT_TRUE(std::filesystem::exists(test_dir_name));
    EXPECT_TRUE(std::filesystem::exists(existing_file));
}

/**
 * @brief Тест генерации уникальных имен файлов
 */
TEST_F(TempFileManagerTest, GenerateUniqueFilenames) {
    TempFileManager manager(test_dir_name);
    
    
    std::string file1 = manager.GenerateTempFilename();
    std::string file2 = manager.GenerateTempFilename();
    std::string file3 = manager.GenerateTempFilename("prefix", ".txt");
    
    
    EXPECT_NE(file1, file2);
    EXPECT_NE(file2, file3);
    EXPECT_NE(file1, file3);
    
    
    EXPECT_TRUE(file1.find("run") != std::string::npos);
    EXPECT_TRUE(file1.find(".bin") != std::string::npos);
    EXPECT_TRUE(file3.find("prefix") != std::string::npos);
    EXPECT_TRUE(file3.find(".txt") != std::string::npos);
    
    
    auto base_path = manager.GetBaseDirPath();
    EXPECT_TRUE(file1.find(base_path.string()) == 0);
    EXPECT_TRUE(file2.find(base_path.string()) == 0);
    EXPECT_TRUE(file3.find(base_path.string()) == 0);
}


/**
 * @brief Тест удаления несуществующих файлов
 */
TEST_F(TempFileManagerTest, CleanupNonExistentFile) {
    TempFileManager manager(test_dir_name);
    
    std::string non_existent = (manager.GetBaseDirPath() / "non_existent.txt").string();
    
    
    EXPECT_NO_THROW(manager.CleanupFile(non_existent));
}

/**
 * @brief Тест работы с множественными менеджерами
 */
TEST_F(TempFileManagerTest, MultipleManagers) {
    std::string dir1 = test_dir_name + "_1";
    std::string dir2 = test_dir_name + "_2";
    
    {
        TempFileManager manager1(dir1);
        TempFileManager manager2(dir2);
        
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
 * @brief Тест семантики перемещения
 */
TEST_F(TempFileManagerTest, MoveSemantics) {
    std::string file_path;
    
    {
        TempFileManager manager1(test_dir_name);
        file_path = manager1.GenerateTempFilename();
        std::ofstream(file_path) << "test";
        
        
        TempFileManager manager2 = std::move(manager1);
        
        EXPECT_TRUE(std::filesystem::exists(test_dir_name));
        EXPECT_TRUE(std::filesystem::exists(file_path));
        
        
        EXPECT_EQ(manager2.GetBaseDirPath(), std::filesystem::current_path() / test_dir_name);
    }
    
    
    EXPECT_FALSE(std::filesystem::exists(test_dir_name));
    EXPECT_FALSE(std::filesystem::exists(file_path));
}

/**
 * @brief Тест обработки ошибок создания директории
 */
TEST_F(TempFileManagerTest, DirectoryCreationError) {
    
    
    
    std::string invalid_dir_name = "/root/impossible_to_create_directory_here_12345";
    
    
    if (std::filesystem::exists("/root") && 
        std::filesystem::status("/root").permissions() != std::filesystem::perms::none) {
        GTEST_SKIP() << "Cannot test directory creation failure on this system";
    }
    
    
    EXPECT_THROW({
        TempFileManager manager(invalid_dir_name);
    }, std::runtime_error);
}
