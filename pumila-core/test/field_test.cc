#include <gtest/gtest.h>
#include <pumila/pumila.h>
#include <stdexcept>

using namespace pumila;

TEST(FieldTest, get) {
    FieldState3 field;
    for (int y = 0; y < 13; y++) {
        for (int x = 0; x < 6; x++) {
            EXPECT_EQ(field.get(x, y), Puyo::none);
        }
    }
    EXPECT_THROW(field.get(7, 0), std::out_of_range);
    EXPECT_THROW(field.get(0, 14), std::out_of_range);
}
TEST(FieldTest, set) {
    FieldState3 field;
    field.set(3, 3, Puyo::green);
    EXPECT_EQ(field.get(3, 3), Puyo::green);
}
