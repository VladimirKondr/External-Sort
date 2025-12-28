/**
 * @file test_element_buffer.cpp
 * @brief Tests for ElementBuffer class
 */

#include "element_buffer.hpp"

#include <gtest/gtest.h>
#include "Registry.hpp"

using io::ElementBuffer;

/**
 * @brief Fixture for ElementBuffer tests
 */
class ElementBufferTest : public ::testing::Test {
protected:
    static constexpr uint64_t kDefaultCapacity = 10;

    void SetUp() override {
        logging::SetDefaultLogger();
        
        buffer_ = std::make_unique<ElementBuffer<int>>(kDefaultCapacity);
    }

    void TearDown() override {
        buffer_.reset();
    }

    std::unique_ptr<ElementBuffer<int>> buffer_;
};

/**
 * @brief Test constructors
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
 * @brief PushBack test
 */
TEST_F(ElementBufferTest, PushBackTest) {
    EXPECT_FALSE(buffer_->PushBack(1));
    EXPECT_EQ(buffer_->Size(), 1);
    EXPECT_FALSE(buffer_->IsEmpty());
    EXPECT_FALSE(buffer_->IsFull());

    for (int i = 2; i <= static_cast<int>(kDefaultCapacity) - 1; ++i) {
        EXPECT_FALSE(buffer_->PushBack(i));
    }

    EXPECT_TRUE(buffer_->PushBack(static_cast<int>(kDefaultCapacity)));
    EXPECT_TRUE(buffer_->IsFull());
    EXPECT_EQ(buffer_->Size(), kDefaultCapacity);

    EXPECT_TRUE(buffer_->PushBack(999));
    EXPECT_EQ(buffer_->Size(), kDefaultCapacity);
}

/**
 * @brief Test access
 */
TEST_F(ElementBufferTest, DataAccessTest) {
    for (int i = 1; i <= 5; ++i) {
        buffer_->PushBack(i);
    }

    const int* data = buffer_->Data();
    ASSERT_NE(data, nullptr);
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(data[i], i + 1);
    }

    int* raw_data = buffer_->RawDataPtr();
    ASSERT_NE(raw_data, nullptr);
    raw_data[0] = 999;
    EXPECT_EQ(buffer_->Data()[0], 999);
}

/**
 * @brief Test read
 */
TEST_F(ElementBufferTest, ReadingTest) {
    for (int i = 1; i <= 5; ++i) {
        buffer_->PushBack(i);
    }

    EXPECT_TRUE(buffer_->HasMoreToRead());
    for (int i = 1; i <= 5; ++i) {
        EXPECT_TRUE(buffer_->HasMoreToRead());
        int value = buffer_->ReadNext();
        EXPECT_EQ(value, i);
    }

    EXPECT_FALSE(buffer_->HasMoreToRead());

    int default_value = buffer_->ReadNext();
    EXPECT_EQ(default_value, int{});
}

/**
 * @brief Test SetValidElementsCount
 */
TEST_F(ElementBufferTest, SetValidElementsCountTest) {
    int* raw_data = buffer_->RawDataPtr();
    for (uint64_t i = 0; i < buffer_->Capacity(); ++i) {
        raw_data[i] = static_cast<int>(i + 100);
    }

    buffer_->SetValidElementsCount(3);
    EXPECT_EQ(buffer_->Size(), 3);
    EXPECT_FALSE(buffer_->IsEmpty());
    EXPECT_FALSE(buffer_->IsFull());

    for (int i = 0; i < 3; ++i) {
        EXPECT_TRUE(buffer_->HasMoreToRead());
        int value = buffer_->ReadNext();
        EXPECT_EQ(value, i + 100);
    }
    EXPECT_FALSE(buffer_->HasMoreToRead());

    EXPECT_THROW(buffer_->SetValidElementsCount(  // NOLINT(cppcoreguidelines-avoid-goto)
                     buffer_->Capacity() + 1),
                 std::length_error);
}

/**
 * @brief Test Clear
 */
TEST_F(ElementBufferTest, ClearTest) {
    for (int i = 1; i <= 5; ++i) {
        buffer_->PushBack(i);
    }
    EXPECT_FALSE(buffer_->IsEmpty());
    EXPECT_EQ(buffer_->Size(), 5);

    buffer_->ReadNext();
    buffer_->ReadNext();

    buffer_->Clear();
    EXPECT_TRUE(buffer_->IsEmpty());
    EXPECT_EQ(buffer_->Size(), 0);
    EXPECT_FALSE(buffer_->HasMoreToRead());

    EXPECT_FALSE(buffer_->PushBack(999));
    EXPECT_EQ(buffer_->Size(), 1);
}

/**
 * @brief Test move
 */
TEST_F(ElementBufferTest, MoveSemantics) {
    for (int i = 1; i <= 5; ++i) {
        buffer_->PushBack(i);
    }

    ElementBuffer<int> moved_buffer = std::move(*buffer_);
    EXPECT_EQ(moved_buffer.Size(), 5);
    EXPECT_EQ(moved_buffer.Capacity(), kDefaultCapacity);

    ElementBuffer<int> assigned_buffer(1);
    assigned_buffer = std::move(moved_buffer);
    EXPECT_EQ(assigned_buffer.Size(), 5);
    EXPECT_EQ(assigned_buffer.Capacity(), kDefaultCapacity);

    for (int i = 1; i <= 5; ++i) {
        int value = assigned_buffer.ReadNext();
        EXPECT_EQ(value, i);
    }
}

/**
 * @brief Test different types
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
