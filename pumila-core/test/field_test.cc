#include "pumila/action.h"
#include "pumila/garbage.h"
#include <gtest/gtest.h>
#include <memory>
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

    for (int x = 0; x < 6; x++) {
        EXPECT_EQ(field.getHeight(x), 0);
    }
}
TEST(FieldTest, set) {
    FieldState3 field;
    field.set(3, 3, Puyo::green);
    EXPECT_EQ(field.get(3, 3), Puyo::green);
}
TEST(FieldTest, next) {
    FieldState3 field(123);
    FieldState3 field2(123);
    EXPECT_EQ(field.getNext(0), field2.getNext(0));
    EXPECT_EQ(field.getNext(1), field2.getNext(1));
    EXPECT_THROW(field.getNext(10), std::out_of_range);

    PuyoPair a{
        Puyo::green, Puyo::green, {5, Action::Rotation::horizontal_left}};
    field.updateNext(a);
    EXPECT_EQ(field.getNext(0), a);

    auto next = field.getNext(1);
    field.putNext();
    EXPECT_EQ(field.getNext(0), next);
}
TEST(FieldTest, garbage) {
    FieldState3 field;
    EXPECT_EQ(field.getGarbageNumTotal(), 0);
    auto garbage = std::make_shared<GarbageGroup>(20);
    auto garbage2 = std::make_shared<GarbageGroup>(50);
    field.addGarbage(garbage);
    EXPECT_EQ(field.getGarbageNumTotal(), 20);
    EXPECT_EQ(garbage->garbageNum(), 20);
    field.addGarbage(garbage2);
    EXPECT_EQ(field.getGarbageNumTotal(), 70);
    EXPECT_EQ(garbage2->garbageNum(), 50);

    EXPECT_EQ(field.calcGarbage(50), 0);
    EXPECT_EQ(field.calcGarbage(50), 1);
    EXPECT_EQ(field.cancelGarbage(1), 1);
    EXPECT_EQ(field.getGarbageNumTotal(), 69);
    EXPECT_EQ(garbage->garbageNum(), 20);
    EXPECT_EQ(garbage->restGarbageNum(), 19);
    EXPECT_EQ(garbage->cancelledNum(), 1);
    EXPECT_EQ(garbage->fellNum(), 0);
    EXPECT_FALSE(garbage->done());
    EXPECT_EQ(garbage2->garbageNum(), 50);
    EXPECT_EQ(garbage2->restGarbageNum(), 50);
    EXPECT_EQ(garbage2->cancelledNum(), 0);
    EXPECT_EQ(garbage2->fellNum(), 0);
    EXPECT_FALSE(garbage2->done());

    std::array<std::pair<std::size_t, std::size_t>, 30> garbage_list;
    std::size_t garbage_num;
    field.putGarbage(&garbage_list, &garbage_num);
    EXPECT_EQ(garbage_num, 30);
    EXPECT_EQ(field.getGarbageNumTotal(), 39);
    EXPECT_EQ(garbage->garbageNum(), 20);
    EXPECT_EQ(garbage->restGarbageNum(), 0);
    EXPECT_EQ(garbage->cancelledNum(), 1);
    EXPECT_EQ(garbage->fellNum(), 19);
    EXPECT_TRUE(garbage->done());
    EXPECT_EQ(garbage2->garbageNum(), 50);
    EXPECT_EQ(garbage2->restGarbageNum(), 39);
    EXPECT_EQ(garbage2->cancelledNum(), 0);
    EXPECT_EQ(garbage2->fellNum(), 11);
    EXPECT_FALSE(garbage2->done());
}
