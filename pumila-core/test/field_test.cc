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

    std::vector<std::pair<std::size_t, std::size_t>> garbage_list;
    field.putGarbage(&garbage_list);
    EXPECT_EQ(garbage_list.size(), 30);
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
TEST(FieldTest, height) {
    FieldState3 field;
    for (int x = 0; x < 6; x++) {
        EXPECT_EQ(field.getHeight(x), 0);
    }
    field.updateNext({Puyo::red, Puyo::red, {2, Action::Rotation::vertical}});
    EXPECT_EQ(field.getNextHeight(),
              (std::make_pair<std::size_t, std::size_t>(0, 1)));
    field.updateNext(
        {Puyo::red, Puyo::red, {2, Action::Rotation::vertical_inverse}});
    EXPECT_EQ(field.getNextHeight(),
              (std::make_pair<std::size_t, std::size_t>(1, 0)));
    field.updateNext(
        {Puyo::red, Puyo::red, {2, Action::Rotation::horizontal_left}});
    EXPECT_EQ(field.getNextHeight(),
              (std::make_pair<std::size_t, std::size_t>(0, 0)));
    field.updateNext(
        {Puyo::red, Puyo::red, {2, Action::Rotation::horizontal_right}});
    EXPECT_EQ(field.getNextHeight(),
              (std::make_pair<std::size_t, std::size_t>(0, 0)));

    field.set(1, 0, Puyo::red);
    field.set(1, 1, Puyo::red);
    field.set(2, 0, Puyo::red);
    field.updateNext({Puyo::red, Puyo::red, {1, Action::Rotation::vertical}});
    EXPECT_EQ(field.getNextHeight(),
              (std::make_pair<std::size_t, std::size_t>(2, 3)));
    field.updateNext(
        {Puyo::red, Puyo::red, {1, Action::Rotation::vertical_inverse}});
    EXPECT_EQ(field.getNextHeight(),
              (std::make_pair<std::size_t, std::size_t>(3, 2)));
    field.updateNext(
        {Puyo::red, Puyo::red, {1, Action::Rotation::horizontal_left}});
    EXPECT_EQ(field.getNextHeight(),
              (std::make_pair<std::size_t, std::size_t>(2, 0)));
    field.updateNext(
        {Puyo::red, Puyo::red, {1, Action::Rotation::horizontal_right}});
    EXPECT_EQ(field.getNextHeight(),
              (std::make_pair<std::size_t, std::size_t>(2, 1)));
}
TEST(FieldTest, put) {
    FieldState3 field;
    field.updateNext({Puyo::red, Puyo::green, {0, Action::Rotation::vertical}});
    field.putNext();
    EXPECT_EQ(field.get(0, 0), Puyo::red);
    EXPECT_EQ(field.get(0, 1), Puyo::green);
    field.updateNext(
        {Puyo::blue, Puyo::yellow, {1, Action::Rotation::horizontal_left}});
    field.putNext();
    EXPECT_EQ(field.get(1, 0), Puyo::blue);
    EXPECT_EQ(field.get(0, 2), Puyo::yellow);
}
TEST(FieldTest, chain) {
    FieldState3 field;
    Chain chain = field.deleteChain(1);
    EXPECT_TRUE(chain.isEmpty());
    std::vector<Chain> chains = field.deleteChainRecurse();
    EXPECT_TRUE(chains.empty());

    field.updateNext({Puyo::red, Puyo::red, {0, Action::Rotation::vertical}});
    field.putNext();
    field.updateNext(
        {Puyo::green, Puyo::green, {1, Action::Rotation::vertical}});
    field.putNext();
    field.updateNext({Puyo::blue, Puyo::blue, {2, Action::Rotation::vertical}});
    field.putNext();
    field.updateNext({Puyo::red, Puyo::red, {1, Action::Rotation::vertical}});
    field.putNext();
    field.updateNext(
        {Puyo::green, Puyo::green, {2, Action::Rotation::vertical}});
    field.putNext();
    field.updateNext({Puyo::blue, Puyo::blue, {3, Action::Rotation::vertical}});
    field.putNext();
    chains = field.deleteChainRecurse();
    ASSERT_EQ(chains.size(), 3);
    EXPECT_EQ(chains[0].connectionNum(), 4);
    EXPECT_EQ(chains[0].score(), 40);
    EXPECT_EQ(chains[1].connectionNum(), 4);
    EXPECT_EQ(chains[1].score(), 320);
    EXPECT_EQ(chains[2].connectionNum(), 4);
    EXPECT_EQ(chains[2].score(), 640);
}
TEST(FieldTest, chainAll) {
    FieldState3 field;
    auto a = field.calcChainAll();
    EXPECT_EQ(a.at(0).at(0), 0);

    field.updateNext({Puyo::red, Puyo::red, {0, Action::Rotation::vertical}});
    field.putNext();
    field.updateNext(
        {Puyo::green, Puyo::green, {1, Action::Rotation::vertical}});
    field.putNext();
    field.updateNext({Puyo::blue, Puyo::blue, {2, Action::Rotation::vertical}});
    field.putNext();
    field.updateNext({Puyo::red, Puyo::red, {1, Action::Rotation::vertical}});
    field.putNext();
    field.updateNext(
        {Puyo::green, Puyo::green, {2, Action::Rotation::vertical}});
    field.putNext();
    a = field.calcChainAll();
    EXPECT_EQ(a.at(0).at(0), 0);
    EXPECT_EQ(a.at(1).at(0), 0);
    EXPECT_EQ(a.at(0).at(1), 1);
    EXPECT_EQ(a.at(1).at(1), 1);
    EXPECT_EQ(a.at(2).at(1), 0);
    EXPECT_EQ(a.at(3).at(1), 0);
    EXPECT_EQ(a.at(0).at(2), 2);
    EXPECT_EQ(a.at(1).at(2), 2);
    EXPECT_EQ(a.at(2).at(2), 0);
    EXPECT_EQ(a.at(3).at(2), 0);
}
