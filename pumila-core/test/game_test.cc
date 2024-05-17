#include <gtest/gtest.h>
#include <memory>
#include <pumila/pumila.h>
#include <stdexcept>

using namespace pumila;

TEST(GameTest, put) {
    auto sim = GameSim::makeNew();
    EXPECT_EQ(sim->phase->get(), GameSim::Phase::free);
    ASSERT_TRUE(sim->field.has_value());
    sim->field->updateNext({Puyo::red, Puyo::green});
    sim->put({0, Action::Rotation::vertical_inverse});
    sim->step();
    ASSERT_NE(sim->phase, nullptr);
    EXPECT_EQ(sim->phase->get(), GameSim::Phase::fall);
    EXPECT_EQ(sim->field->get(0, 0), Puyo::green);
    EXPECT_EQ(sim->field->get(0, 1), Puyo::red);
}
