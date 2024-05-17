#include "pumila/game.h"
#include <gtest/gtest.h>
#include <memory>
#include <pumila/pumila.h>
#include <stdexcept>

using namespace pumila;

TEST(GameTest, put) {
    auto sim = GameSim::makeNew();
    EXPECT_EQ(sim->phase->get(), GameSim::Phase::free);
    ASSERT_TRUE(sim->field.has_value());
    EXPECT_NE(sim->current_step, nullptr);
    auto current_step = sim->current_step;

    sim->field->updateNext({Puyo::red, Puyo::green});
    sim->put({0, Action::Rotation::vertical_inverse});
    sim->step();
    ASSERT_NE(sim->phase, nullptr);
    EXPECT_EQ(sim->phase->get(), GameSim::Phase::fall);
    EXPECT_EQ(sim->field->get(0, 0), Puyo::green);
    EXPECT_EQ(sim->field->get(0, 1), Puyo::red);
    EXPECT_EQ(sim->current_step, current_step);
    EXPECT_EQ(current_step->field_before.get(0, 0), Puyo::none);
    EXPECT_EQ(current_step->field_before.get(0, 1), Puyo::none);
    EXPECT_FALSE(current_step->field_after.has_value());
    EXPECT_TRUE(current_step->chains.empty());
    EXPECT_EQ(current_step->garbage_send, nullptr);
    EXPECT_FALSE(current_step->op_field_before.has_value());
    EXPECT_FALSE(current_step->op_field_after.has_value());
    EXPECT_TRUE(current_step->garbage_recv.empty());
    EXPECT_TRUE(current_step->garbage_fell_pos.empty());

    while (sim->phase && sim->phase->get() == GameSim::Phase::fall) {
        ASSERT_NE(sim->phase, nullptr);
        sim->step();
    }
    EXPECT_EQ(sim->current_step, current_step);
    EXPECT_EQ(current_step->field_before.get(0, 0), Puyo::none);
    EXPECT_EQ(current_step->field_before.get(0, 1), Puyo::none);
    EXPECT_FALSE(current_step->field_after.has_value());
    EXPECT_TRUE(current_step->chains.empty());
    EXPECT_EQ(current_step->garbage_send, nullptr);
    EXPECT_FALSE(current_step->op_field_before.has_value());
    EXPECT_FALSE(current_step->op_field_after.has_value());
    EXPECT_TRUE(current_step->garbage_recv.empty());
    EXPECT_TRUE(current_step->garbage_fell_pos.empty());

    while (sim->phase && sim->phase->get() == GameSim::Phase::garbage) {
        ASSERT_NE(sim->phase, nullptr);
        sim->step();
    }
    EXPECT_EQ(sim->phase->get(), GameSim::Phase::free);
    EXPECT_NE(sim->current_step, current_step);
    EXPECT_EQ(current_step->field_before.get(0, 0), Puyo::none);
    EXPECT_EQ(current_step->field_before.get(0, 1), Puyo::none);
    ASSERT_TRUE(current_step->field_after.has_value());
    EXPECT_EQ(current_step->field_after->get(0, 0), Puyo::green);
    EXPECT_EQ(current_step->field_after->get(0, 1), Puyo::red);
    EXPECT_TRUE(current_step->chains.empty());
    EXPECT_EQ(current_step->garbage_send, nullptr);
    EXPECT_FALSE(current_step->op_field_before.has_value());
    EXPECT_FALSE(current_step->op_field_after.has_value());
    EXPECT_TRUE(current_step->garbage_recv.empty());
    EXPECT_TRUE(current_step->garbage_fell_pos.empty());
}

TEST(GameTest, chain) {
    auto sim = GameSim::makeNew();
    auto sim2 = GameSim::makeNew();
    sim->setOpponentSim(sim2);

    sim->field->updateNext({Puyo::red, Puyo::red});
    sim->put({0, Action::Rotation::vertical});
    do {
        sim->step();
    } while (sim->phase->get() != GameSim::Phase::free);
    sim->field->updateNext({Puyo::green, Puyo::green});
    sim->put({1, Action::Rotation::vertical});
    do {
        sim->step();
    } while (sim->phase->get() != GameSim::Phase::free);
    sim->field->updateNext({Puyo::blue, Puyo::blue});
    sim->put({2, Action::Rotation::vertical});
    do {
        sim->step();
    } while (sim->phase->get() != GameSim::Phase::free);
    sim->field->updateNext({Puyo::red, Puyo::red});
    sim->put({1, Action::Rotation::vertical});
    do {
        sim->step();
    } while (sim->phase->get() != GameSim::Phase::free);
    sim->field->updateNext({Puyo::green, Puyo::green});
    sim->put({2, Action::Rotation::vertical});
    do {
        sim->step();
    } while (sim->phase->get() != GameSim::Phase::free);
    sim->field->updateNext({Puyo::blue, Puyo::blue});
    sim->put({3, Action::Rotation::vertical});

    sim->step();
    auto current_step = sim->current_step;
    EXPECT_EQ(current_step->chains.size(), 3);
    EXPECT_EQ(current_step->garbage_send, nullptr);

    while (sim->phase->get() == GameSim::Phase::fall) {
        sim->step();
    }
    EXPECT_EQ(current_step->chains.size(), 3);
    ASSERT_NE(current_step->garbage_send, nullptr);
    constexpr int garbage_num = (40 + 320 + 640) / 70;
    EXPECT_EQ(current_step->garbage_send->garbageNum(), garbage_num);
    EXPECT_EQ(current_step->garbage_send->restGarbageNum(), garbage_num);
    EXPECT_FALSE(current_step->garbage_send->done());
    EXPECT_EQ(sim2->field->getGarbageNumTotal(), garbage_num);

    ASSERT_NE(sim2->current_step, nullptr);
    EXPECT_EQ(sim2->current_step->garbage_recv.size(), 1);
    EXPECT_EQ(current_step->garbage_send, sim2->current_step->garbage_recv[0]);
}
