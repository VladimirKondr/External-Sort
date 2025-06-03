/**
 * @file test_template_types.cpp
 * @brief Тесты для проверки шаблонности библиотеки внешней сортировки
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
 * @brief Базовый класс для шаблонных тестов с файловым хранилищем
 * @tparam T Тип тестируемых данных
 */
template <typename T>
class TemplateTypeTestBase : public ::testing::Test {
   protected:
    void SetUp() override {
        test_dir = "template_test_" + GetTypeName() + "_" + std::to_string(GetTestCount());
        if (std::filesystem::exists(test_dir)) {
            std::filesystem::remove_all(test_dir);
        }
        std::filesystem::create_directory(test_dir);
        factory = std::make_unique<FileStreamFactory<T>>(test_dir);
    }

    void TearDown() override {
        factory.reset();
        if (std::filesystem::exists(test_dir)) {
            std::filesystem::remove_all(test_dir);
        }
    }

    std::string test_dir;
    std::unique_ptr<FileStreamFactory<T>> factory;

    void CreateTestData(const std::string& storage_id, const std::vector<T>& data) {
        auto output = factory->CreateOutputStream(storage_id, 100);
        for (const T& value : data) {
            output->Write(value);
        }
        output->Finalize();
    }

    std::vector<T> ReadAllData(const std::string& storage_id) {
        auto input = factory->CreateInputStream(storage_id, 100);
        std::vector<T> result;
        while (!input->IsExhausted()) {
            result.push_back(input->Value());
            input->Advance();
        }
        return result;
    }

    bool IsSorted(const std::vector<T>& data, bool ascending = true) {
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

   private:
    static int& GetTestCount() {
        static int count = 0;
        return ++count;
    }

    std::string GetTypeName() const {
        if constexpr (std::is_same_v<T, int8_t>) {
            return "int8";
        } else if constexpr (std::is_same_v<T, uint8_t>) {
            return "uint8";
        } else if constexpr (std::is_same_v<T, int16_t>) {
            return "int16";
        } else if constexpr (std::is_same_v<T, uint16_t>) {
            return "uint16";
        } else if constexpr (std::is_same_v<T, int32_t>) {
            return "int32";
        } else if constexpr (std::is_same_v<T, uint32_t>) {
            return "uint32";
        } else if constexpr (std::is_same_v<T, int64_t>) {
            return "int64";
        } else if constexpr (std::is_same_v<T, uint64_t>) {
            return "uint64";
        } else if constexpr (std::is_same_v<T, float>) {
            return "float";
        } else if constexpr (std::is_same_v<T, double>) {
            return "double";
        } else {
            return "custom";
        }
    }
};

/**
 * @brief Тестирование с типом int8_t (8-битные знаковые числа)
 */
class Int8Test : public TemplateTypeTestBase<int8_t> {};

TEST_F(Int8Test, BasicSorting) {
    const std::string input_id = "int8_input";
    const std::string output_id = "int8_output";

    std::vector<int8_t> test_data = {127, -128, 0, 42, -1, 100, -50};
    CreateTestData(input_id, test_data);

    KWayMergeSorter<int8_t> sorter(*factory, input_id, output_id, sizeof(int8_t) * 4, 2, 10, true);
    sorter.Sort();

    std::vector<int8_t> result = ReadAllData(output_id);
    std::vector<int8_t> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, true));
}

/**
 * @brief Тестирование с типом uint8_t (8-битные беззнаковые числа)
 */
class UInt8Test : public TemplateTypeTestBase<uint8_t> {};

TEST_F(UInt8Test, BasicSorting) {
    const std::string input_id = "uint8_input";
    const std::string output_id = "uint8_output";

    std::vector<uint8_t> test_data = {255, 0, 128, 1, 127, 200, 50};
    CreateTestData(input_id, test_data);

    KWayMergeSorter<uint8_t> sorter(
        *factory, input_id, output_id, sizeof(uint8_t) * 4, 2, 10, true);
    sorter.Sort();

    std::vector<uint8_t> result = ReadAllData(output_id);
    std::vector<uint8_t> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, true));
}

/**
 * @brief Тестирование с типом int16_t (16-битные знаковые числа)
 */
class Int16Test : public TemplateTypeTestBase<int16_t> {};

TEST_F(Int16Test, BasicSorting) {
    const std::string input_id = "int16_input";
    const std::string output_id = "int16_output";

    std::vector<int16_t> test_data = {32'767, -32'768, 0, 1000, -1000, 12'345, -5432};
    CreateTestData(input_id, test_data);

    KWayMergeSorter<int16_t> sorter(
        *factory, input_id, output_id, sizeof(int16_t) * 4, 2, 10, true);
    sorter.Sort();

    std::vector<int16_t> result = ReadAllData(output_id);
    std::vector<int16_t> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, true));
}

/**
 * @brief Тестирование с типом uint16_t (16-битные беззнаковые числа)
 */
class UInt16Test : public TemplateTypeTestBase<uint16_t> {};

TEST_F(UInt16Test, BasicSorting) {
    const std::string input_id = "uint16_input";
    const std::string output_id = "uint16_output";

    std::vector<uint16_t> test_data = {65'535, 0, 32'768, 1, 12'345, 54'321, 9999};
    CreateTestData(input_id, test_data);

    KWayMergeSorter<uint16_t> sorter(
        *factory, input_id, output_id, sizeof(uint16_t) * 4, 2, 10, true);
    sorter.Sort();

    std::vector<uint16_t> result = ReadAllData(output_id);
    std::vector<uint16_t> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, true));
}

/**
 * @brief Тестирование с типом int32_t (32-битные знаковые числа)
 */
class Int32Test : public TemplateTypeTestBase<int32_t> {};

TEST_F(Int32Test, BasicSorting) {
    const std::string input_id = "int32_input";
    const std::string output_id = "int32_output";

    std::vector<int32_t> test_data = {
      2'147'483'647, -2'147'483'648, 0, 1'000'000, -1'000'000, 123'456'789, -987'654'321};
    CreateTestData(input_id, test_data);

    KWayMergeSorter<int32_t> sorter(
        *factory, input_id, output_id, sizeof(int32_t) * 4, 2, 10, true);
    sorter.Sort();

    std::vector<int32_t> result = ReadAllData(output_id);
    std::vector<int32_t> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, true));
}

/**
 * @brief Тестирование с типом uint32_t (32-битные беззнаковые числа)
 */
class UInt32Test : public TemplateTypeTestBase<uint32_t> {};

TEST_F(UInt32Test, BasicSorting) {
    const std::string input_id = "uint32_input";
    const std::string output_id = "uint32_output";

    std::vector<uint32_t> test_data = {
      4'294'967'295U, 0, 2'147'483'648U, 1, 1'000'000'000U, 123'456'789, 987'654'321};
    CreateTestData(input_id, test_data);

    KWayMergeSorter<uint32_t> sorter(
        *factory, input_id, output_id, sizeof(uint32_t) * 4, 2, 10, true);
    sorter.Sort();

    std::vector<uint32_t> result = ReadAllData(output_id);
    std::vector<uint32_t> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, true));
}

/**
 * @brief Тестирование с типом uint64_t (64-битные беззнаковые числа)
 */
class UInt64Test : public TemplateTypeTestBase<uint64_t> {};

TEST_F(UInt64Test, BasicSorting) {
    const std::string input_id = "uint64_input";
    const std::string output_id = "uint64_output";

    std::vector<uint64_t> test_data = {
      18'446'744'073'709'551'615ULL, 0,
      9'223'372'036'854'775'808ULL,  1,
      1'000'000'000'000ULL,          123'456'789'012'345ULL,
      987'654'321'098'765ULL};
    CreateTestData(input_id, test_data);

    KWayMergeSorter<uint64_t> sorter(
        *factory, input_id, output_id, sizeof(uint64_t) * 4, 2, 10, true);
    sorter.Sort();

    std::vector<uint64_t> result = ReadAllData(output_id);
    std::vector<uint64_t> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, true));
}

/**
 * @brief Тестирование с типом int64_t (64-битные знаковые числа)
 */
class Int64Test : public TemplateTypeTestBase<int64_t> {};

TEST_F(Int64Test, BasicSorting) {
    const std::string input_id = "int64_input";
    const std::string output_id = "int64_output";

    std::vector<int64_t> test_data = {
      9'223'372'036'854'775'807LL,
      (-9'223'372'036'854'775'807LL - 1),
      0,
      1'000'000'000'000LL,
      -1'000'000'000'000LL,
      123'456'789'012'345LL,
      -987'654'321'098'765LL};
    CreateTestData(input_id, test_data);

    KWayMergeSorter<int64_t> sorter(
        *factory, input_id, output_id, sizeof(int64_t) * 4, 2, 10, true);
    sorter.Sort();

    std::vector<int64_t> result = ReadAllData(output_id);
    std::vector<int64_t> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, true));
}

/**
 * @brief Тестирование с типом float (32-битные числа с плавающей точкой)
 */
class FloatTest : public TemplateTypeTestBase<float> {};

TEST_F(FloatTest, BasicSorting) {
    const std::string input_id = "float_input";
    const std::string output_id = "float_output";

    std::vector<float> test_data = {
      3.14159f, -2.71828f, 0.0f, 1.41421f, -1.73205f, 2.23607f, -3.16227f};
    CreateTestData(input_id, test_data);

    KWayMergeSorter<float> sorter(*factory, input_id, output_id, sizeof(float) * 4, 2, 10, true);
    sorter.Sort();

    std::vector<float> result = ReadAllData(output_id);
    std::vector<float> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, true));
}

/**
 * @brief Тестирование с типом double (64-битные числа с плавающей точкой)
 */
class DoubleTest : public TemplateTypeTestBase<double> {};

TEST_F(DoubleTest, BasicSorting) {
    const std::string input_id = "double_input";
    const std::string output_id = "double_output";

    std::vector<double> test_data = {
      3.141592653589793,  -2.718281828459045, 0.0, 1.4142135623730951, -1.7320508075688772,
      2.2360679774997896, -3.1622776601683795};
    CreateTestData(input_id, test_data);

    KWayMergeSorter<double> sorter(*factory, input_id, output_id, sizeof(double) * 4, 2, 10, true);
    sorter.Sort();

    std::vector<double> result = ReadAllData(output_id);
    std::vector<double> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, true));
}

/**
 * @brief Простая структура для тестирования пользовательских типов
 */
struct TestStruct {
    int id;
    double value;

    bool operator<(const TestStruct& other) const {
        return id < other.id;
    }

    bool operator>(const TestStruct& other) const {
        return id > other.id;
    }

    bool operator==(const TestStruct& other) const {
        return id == other.id && value == other.value;
    }

    bool operator!=(const TestStruct& other) const {
        return !(*this == other);
    }
};

/**
 * @brief Тестирование с пользовательским типом данных
 */
class CustomStructTest : public TemplateTypeTestBase<TestStruct> {};

TEST_F(CustomStructTest, BasicSorting) {
    const std::string input_id = "struct_input";
    const std::string output_id = "struct_output";

    std::vector<TestStruct> test_data = {
      {5, 3.14}, {2, 2.71}, {8, 1.41}, {1, 1.73}, {7, 2.23}, {3, 3.16}, {6, 1.61}};
    CreateTestData(input_id, test_data);

    KWayMergeSorter<TestStruct> sorter(
        *factory, input_id, output_id, sizeof(TestStruct) * 4, 2, 10, true);
    sorter.Sort();

    std::vector<TestStruct> result = ReadAllData(output_id);
    std::vector<TestStruct> expected = test_data;
    std::sort(expected.begin(), expected.end());

    EXPECT_EQ(result, expected);
    EXPECT_TRUE(IsSorted(result, true));
}

/**
 * @brief Тест сортировки по убыванию для разных типов с файловыми потоками
 */
TEST(TemplateTypesDescending, VariousTypesFile) {
    const std::string test_dir = "template_descending_test";

    if (std::filesystem::exists(test_dir)) {
        std::filesystem::remove_all(test_dir);
    }
    std::filesystem::create_directory(test_dir);

    {
        FileStreamFactory<int> factory(test_dir);
        std::vector<int> test_data = {1, 5, 3, 9, 2, 7, 4};

        auto output = factory.CreateOutputStream("input_int", 100);
        for (int value : test_data) {
            output->Write(value);
        }
        output->Finalize();

        KWayMergeSorter<int> sorter(
            factory, "input_int", "output_int", sizeof(int) * 4, 2, 10, false);
        sorter.Sort();

        auto input = factory.CreateInputStream("output_int", 100);
        std::vector<int> result;
        while (!input->IsExhausted()) {
            result.push_back(input->Value());
            input->Advance();
        }

        std::vector<int> expected = test_data;
        std::sort(expected.rbegin(), expected.rend());
        EXPECT_EQ(result, expected);
    }

    {
        FileStreamFactory<double> factory(test_dir);
        std::vector<double> test_data = {1.1, 5.5, 3.3, 9.9, 2.2, 7.7, 4.4};

        auto output = factory.CreateOutputStream("input_double", 100);
        for (double value : test_data) {
            output->Write(value);
        }
        output->Finalize();

        KWayMergeSorter<double> sorter(
            factory, "input_double", "output_double", sizeof(double) * 4, 2, 10, false);
        sorter.Sort();

        auto input = factory.CreateInputStream("output_double", 100);
        std::vector<double> result;
        while (!input->IsExhausted()) {
            result.push_back(input->Value());
            input->Advance();
        }

        std::vector<double> expected = test_data;
        std::sort(expected.rbegin(), expected.rend());
        EXPECT_EQ(result, expected);
    }

    if (std::filesystem::exists(test_dir)) {
        std::filesystem::remove_all(test_dir);
    }
}

/**
 * @brief Стресс-тест с большими данными для разных типов с файловыми потоками
 */
TEST(TemplateTypesStress, LargeDatasetsFile) {
    const uint64_t test_size = 1000;
    const std::string test_dir = "template_stress_test";

    if (std::filesystem::exists(test_dir)) {
        std::filesystem::remove_all(test_dir);
    }
    std::filesystem::create_directory(test_dir);

    {
        FileStreamFactory<uint32_t> factory(test_dir);
        std::vector<uint32_t> test_data;
        test_data.reserve(test_size);

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint32_t> dis(0, 1'000'000);

        for (uint64_t i = 0; i < test_size; ++i) {
            test_data.push_back(dis(gen));
        }

        auto output = factory.CreateOutputStream("stress_input", 100);
        for (uint32_t value : test_data) {
            output->Write(value);
        }
        output->Finalize();

        KWayMergeSorter<uint32_t> sorter(
            factory, "stress_input", "stress_output", sizeof(uint32_t) * 20, 4, 50, true);
        sorter.Sort();

        auto input = factory.CreateInputStream("stress_output", 100);
        std::vector<uint32_t> result;
        while (!input->IsExhausted()) {
            result.push_back(input->Value());
            input->Advance();
        }

        std::vector<uint32_t> expected = test_data;
        std::sort(expected.begin(), expected.end());

        EXPECT_EQ(result.size(), expected.size());
        EXPECT_EQ(result, expected);

        for (uint64_t i = 1; i < result.size(); ++i) {
            EXPECT_LE(result[i - 1], result[i]);
        }
    }

    {
        FileStreamFactory<float> factory(test_dir);
        std::vector<float> test_data;
        test_data.reserve(test_size);

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(-1000.0f, 1000.0f);

        for (uint64_t i = 0; i < test_size; ++i) {
            test_data.push_back(dis(gen));
        }

        auto output = factory.CreateOutputStream("stress_float_input", 100);
        for (float value : test_data) {
            output->Write(value);
        }
        output->Finalize();

        KWayMergeSorter<float> sorter(
            factory, "stress_float_input", "stress_float_output", sizeof(float) * 20, 4, 50, true);
        sorter.Sort();

        auto input = factory.CreateInputStream("stress_float_output", 100);
        std::vector<float> result;
        while (!input->IsExhausted()) {
            result.push_back(input->Value());
            input->Advance();
        }

        std::vector<float> expected = test_data;
        std::sort(expected.begin(), expected.end());

        EXPECT_EQ(result.size(), expected.size());
        EXPECT_EQ(result, expected);

        for (uint64_t i = 1; i < result.size(); ++i) {
            EXPECT_LE(result[i - 1], result[i]);
        }
    }

    if (std::filesystem::exists(test_dir)) {
        std::filesystem::remove_all(test_dir);
    }
}

/**
 * @brief Тест с файловыми потоками для разных типов
 */
TEST(TemplateTypesFile, FileStreamVariousTypes) {
    const std::string test_dir = "template_test_dir";

    if (std::filesystem::exists(test_dir)) {
        std::filesystem::remove_all(test_dir);
    }
    std::filesystem::create_directory(test_dir);

    {
        FileStreamFactory<int16_t> factory(test_dir);
        const std::string input_file = "input_int16.bin";
        const std::string output_file = "output_int16.bin";

        std::vector<int16_t> test_data = {1000, -500, 2000, -1500, 800, -200, 1200};

        auto output = factory.CreateOutputStream(input_file, 100);
        for (int16_t value : test_data) {
            output->Write(value);
        }
        output->Finalize();

        KWayMergeSorter<int16_t> sorter(
            factory, input_file, output_file, sizeof(int16_t) * 4, 2, 10, true);
        sorter.Sort();

        auto input = factory.CreateInputStream(output_file, 100);
        std::vector<int16_t> result;
        while (!input->IsExhausted()) {
            result.push_back(input->Value());
            input->Advance();
        }

        std::vector<int16_t> expected = test_data;
        std::sort(expected.begin(), expected.end());
        EXPECT_EQ(result, expected);
    }

    if (std::filesystem::exists(test_dir)) {
        std::filesystem::remove_all(test_dir);
    }
}
