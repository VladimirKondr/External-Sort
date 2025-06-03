/**
 * @file test_element_buffer.cpp
 * @brief Тесты для класса ElementBuffer
 * @author External Sort Library
 * @version 1.0
 */

#include "element_buffer.hpp"

#include <gtest/gtest.h>

using namespace external_sort;

/**
 * @brief Фикстура для тестов ElementBuffer
 */
class ElementBufferTest : public ::testing::Test {
   protected:
    static constexpr uint64_t DEFAULT_CAPACITY = 10;

    void SetUp() override {
        buffer = std::make_unique<ElementBuffer<int>>(DEFAULT_CAPACITY);
    }

    void TearDown() override {
        buffer.reset();
    }

    std::unique_ptr<ElementBuffer<int>> buffer;
};

/**
 * @brief Тест конструктора с различными размерами емкости
 */
TEST_F(ElementBufferTest, ConstructorTest) {
    ElementBuffer<int> buf1(5);
    EXPECT_EQ(buf1.Capacity(), 5);
    EXPECT_TRUE(buf1.IsEmpty());
    EXPECT_FALSE(buf1.IsFull());
    EXPECT_EQ(buf1.Size(), 0);

    ElementBuffer<int> buf2(0);
    EXPECT_EQ(buf2.Capacity(), 1);

    ElementBuffer<int> buf3(1000);
    EXPECT_EQ(buf3.Capacity(), 1000);
}

/**
 * @brief Тест добавления элементов
 */
TEST_F(ElementBufferTest, PushBackTest) {
    EXPECT_FALSE(buffer->PushBack(1));
    EXPECT_EQ(buffer->Size(), 1);
    EXPECT_FALSE(buffer->IsEmpty());
    EXPECT_FALSE(buffer->IsFull());

    for (int i = 2; i <= static_cast<int>(DEFAULT_CAPACITY) - 1; ++i) {
        EXPECT_FALSE(buffer->PushBack(i));
    }

    EXPECT_TRUE(buffer->PushBack(static_cast<int>(DEFAULT_CAPACITY)));
    EXPECT_TRUE(buffer->IsFull());
    EXPECT_EQ(buffer->Size(), DEFAULT_CAPACITY);

    EXPECT_TRUE(buffer->PushBack(999));
    EXPECT_EQ(buffer->Size(), DEFAULT_CAPACITY);
}

/**
 * @brief Тест доступа к данным
 */
TEST_F(ElementBufferTest, DataAccessTest) {
    for (int i = 1; i <= 5; ++i) {
        buffer->PushBack(i);
    }

    const int* data = buffer->Data();
    ASSERT_NE(data, nullptr);
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(data[i], i + 1);
    }

    int* raw_data = buffer->RawDataPtr();
    ASSERT_NE(raw_data, nullptr);
    raw_data[0] = 999;
    EXPECT_EQ(buffer->Data()[0], 999);
}

/**
 * @brief Тест чтения элементов
 */
TEST_F(ElementBufferTest, ReadingTest) {
    for (int i = 1; i <= 5; ++i) {
        buffer->PushBack(i);
    }

    EXPECT_TRUE(buffer->HasMoreToRead());
    for (int i = 1; i <= 5; ++i) {
        EXPECT_TRUE(buffer->HasMoreToRead());
        int value = buffer->ReadNext();
        EXPECT_EQ(value, i);
    }

    EXPECT_FALSE(buffer->HasMoreToRead());

    int default_value = buffer->ReadNext();
    EXPECT_EQ(default_value, int{});
}

/**
 * @brief Тест установки количества валидных элементов
 */
TEST_F(ElementBufferTest, SetValidElementsCountTest) {
    int* raw_data = buffer->RawDataPtr();
    for (uint64_t i = 0; i < buffer->Capacity(); ++i) {
        raw_data[i] = static_cast<int>(i + 100);
    }

    buffer->SetValidElementsCount(3);
    EXPECT_EQ(buffer->Size(), 3);
    EXPECT_FALSE(buffer->IsEmpty());
    EXPECT_FALSE(buffer->IsFull());

    for (int i = 0; i < 3; ++i) {
        EXPECT_TRUE(buffer->HasMoreToRead());
        int value = buffer->ReadNext();
        EXPECT_EQ(value, i + 100);
    }
    EXPECT_FALSE(buffer->HasMoreToRead());

    EXPECT_THROW(buffer->SetValidElementsCount(buffer->Capacity() + 1), std::length_error);
}

/**
 * @brief Тест очистки буфера
 */
TEST_F(ElementBufferTest, ClearTest) {
    for (int i = 1; i <= 5; ++i) {
        buffer->PushBack(i);
    }
    EXPECT_FALSE(buffer->IsEmpty());
    EXPECT_EQ(buffer->Size(), 5);

    buffer->ReadNext();
    buffer->ReadNext();

    buffer->Clear();
    EXPECT_TRUE(buffer->IsEmpty());
    EXPECT_EQ(buffer->Size(), 0);
    EXPECT_FALSE(buffer->HasMoreToRead());

    EXPECT_FALSE(buffer->PushBack(999));
    EXPECT_EQ(buffer->Size(), 1);
}

/**
 * @brief Тест семантики перемещения
 */
TEST_F(ElementBufferTest, MoveSemantics) {
    for (int i = 1; i <= 5; ++i) {
        buffer->PushBack(i);
    }

    ElementBuffer<int> moved_buffer = std::move(*buffer);
    EXPECT_EQ(moved_buffer.Size(), 5);
    EXPECT_EQ(moved_buffer.Capacity(), DEFAULT_CAPACITY);

    ElementBuffer<int> assigned_buffer(1);
    assigned_buffer = std::move(moved_buffer);
    EXPECT_EQ(assigned_buffer.Size(), 5);
    EXPECT_EQ(assigned_buffer.Capacity(), DEFAULT_CAPACITY);

    for (int i = 1; i <= 5; ++i) {
        int value = assigned_buffer.ReadNext();
        EXPECT_EQ(value, i);
    }
}

/**
 * @brief Тест работы с различными типами данных
 */
TEST(ElementBufferGenericTest, DifferentTypes) {
    ElementBuffer<double> double_buffer(3);
    double_buffer.PushBack(3.14);
    double_buffer.PushBack(2.71);
    EXPECT_DOUBLE_EQ(double_buffer.ReadNext(), 3.14);
    EXPECT_DOUBLE_EQ(double_buffer.ReadNext(), 2.71);

    ElementBuffer<std::string> string_buffer(2);
    string_buffer.PushBack("hello");
    string_buffer.PushBack("world");
    EXPECT_EQ(string_buffer.ReadNext(), "hello");
    EXPECT_EQ(string_buffer.ReadNext(), "world");

    struct TestStruct {
        int x, y;

        bool operator==(const TestStruct& other) const {
            return x == other.x && y == other.y;
        }
    };

    ElementBuffer<TestStruct> struct_buffer(2);
    struct_buffer.PushBack({1, 2});
    struct_buffer.PushBack({3, 4});

    TestStruct s1 = struct_buffer.ReadNext();
    TestStruct s2 = struct_buffer.ReadNext();
    EXPECT_EQ(s1, (TestStruct{1, 2}));
    EXPECT_EQ(s2, (TestStruct{3, 4}));
}
