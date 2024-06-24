#include "pumila/garbage.h"
#include "pumila/step.h"
#include <memory>
#include <pumila/game.h>
#include <cassert>

namespace PUMILA_NS {
GameSim::GameSim(typename std::mt19937::result_type seed, bool enable_garbage)
    : enable_garbage(enable_garbage), opponent(), field(std::nullopt),
      phase(nullptr), current_step(nullptr), prev_step(nullptr) {
    /*if (model) {
        model_action_thread = std::make_optional<std::thread>([this] {
            while (running.load()) {
                while (running.load() && !isFreePhase()) {
                    std::this_thread::yield();
                }
                std::optional<FieldState2> field_copy;
                {
                    std::shared_lock lock(field_m);
                    if (field) {
                        field_copy = *field2();
                    }
                }
                if (field_copy) {
                    softPut(actions.at(this->model->getAction(*field_copy)));
                    while (running.load() && isFreePhase()) {
                        std::this_thread::yield();
                    }
                }
            }
        });
    }*/
    reset(seed);
}

void GameSim::reset(typename std::mt19937::result_type seed) {
    // std::lock_guard lock(step_m);
    // std::lock_guard lock2(field_m);
    field.emplace(seed);
    phase = std::make_unique<GameSim::FreePhase>(this);
    step_count = 0;
}

// void GameSim::stopAction() {
//     running.store(false);
//     if (model_action_thread) {
//         model_action_thread->join();
//         model_action_thread = std::nullopt;
//     }
// }

void GameSim::setOpponentSim(const std::shared_ptr<GameSim> &opponent_s) {
    auto prev_opponent_s = opponent.lock();
    if (prev_opponent_s) {
        prev_opponent_s->opponent.reset();
    }
    opponent = opponent_s;
    if (opponent_s) {
        opponent_s->opponent = shared_from_this();
    }
}

// std::shared_ptr<FieldState> GameSim::field1() {
//     std::shared_lock lock(field_m);
//     return std::make_shared<FieldState>(*field2());
// }

void GameSim::movePair(int dx) {
    // std::lock_guard lock(step_m);
    if (phase && phase->get() == Phase::free) {
        // std::lock_guard lock_f(field_m);
        auto pp = field->getNext(0);
        if (FieldState3::inRange(pp.bottomX() + dx) &&
            FieldState3::inRange(pp.topX() + dx) &&
            (pp.bottomY() > 12 ||
             field->get(pp.bottomX() + dx, pp.bottomY()) == Puyo::none) &&
            (pp.topY() > 12 ||
             field->get(pp.topX() + dx, pp.topY()) == Puyo::none)) {
            pp.x += dx;
            field->updateNext(pp);
        }
    }
}
void GameSim::rotPair(int r) {
    // std::lock_guard lock(step_m);
    if (phase && phase->get() == Phase::free &&
        rot_fail_count < ROT_FAIL_COUNT) {
        // std::lock_guard lock_f(field_m);
        PuyoPair new_pp = field->getNext(0);
        if (r == rot_fail) {
            r *= 2;
        }
        rot_fail = 0;
        new_pp.rotate(r);
        if (!field->checkNextCollision(new_pp)) {
            field->updateNext(new_pp);
            return;
        }
        PuyoPair new_pp2 = new_pp;
        rot_fail_count++;
        new_pp2.y = new_pp.y + 1;
        if (!field->checkNextCollision(new_pp2)) {
            field->updateNext(new_pp2);
            return;
        }
        new_pp2.y = new_pp.y;
        new_pp2.x = new_pp.x + 1;
        if (!field->checkNextCollision(new_pp2)) {
            field->updateNext(new_pp2);
            return;
        }
        new_pp2.y = new_pp.y;
        new_pp2.x = new_pp.x - 1;
        if (!field->checkNextCollision(new_pp2)) {
            field->updateNext(new_pp2);
            return;
        }
        rot_fail = r;
    }
}
void GameSim::quickDrop() {
    // std::lock_guard lock(step_m);
    if (phase && phase->get() == Phase::free) {
        // std::lock_guard lock_f(field_m);
        PuyoPair pp = field->getNext(0);
        pp.y = -1;
        field->updateNext(pp);
        auto f_phase = dynamic_cast<FreePhase *>(phase.get());
        f_phase->put_t = 0;
    }
}
void GameSim::softDrop() {
    // std::lock_guard lock(step_m);
    if (phase && phase->get() == Phase::free) {
        // std::lock_guard lock_f(field_m);
        PuyoPair pp = field->getNext(0);
        pp.y -= FreePhase::SOFT_SPEED / 60;
        field->updateNext(pp);
        auto f_phase = dynamic_cast<FreePhase *>(phase.get());
        f_phase->put_t -= 10;
    }
}

void GameSim::step() {
    // std::lock_guard lock(step_m);
    if (soft_put_target) {
        PuyoPair pp;
        // std::shared_lock lock_f(field_m);
        pp = field->getNext(0);
        if (static_cast<Action>(pp) == soft_put_target) {
            softDrop();
        } else {
            soft_put_cnt--;
            if (soft_put_cnt <= 0) {
                soft_put_cnt = soft_put_interval;
                if ((static_cast<int>(soft_put_target->rot) -
                     static_cast<int>(pp.rot) + 4) %
                        4 ==
                    1) {
                    rotPair(1);
                } else if (soft_put_target->rot != pp.rot) {
                    rotPair(-1);
                }
                if (pp.x < soft_put_target->x) {
                    movePair(1);
                } else if (pp.x > soft_put_target->x) {
                    movePair(-1);
                }
            }
        }
    }
    std::unique_ptr<Phase> next_phase = phase->step();
    if (next_phase) {
        phase = std::move(next_phase);
    }
}
void GameSim::put(const Action &action) {
    // std::lock_guard lock(step_m);
    if (phase && phase->get() == Phase::free) {
        // std::lock_guard lock_f(field_m);
        PuyoPair pp = field->getNext(0);
        field->updateNext({pp, action});
        quickDrop();
        // step();
    }
}
void GameSim::softPut(const Action &action) {
    // std::lock_guard lock(step_m);
    soft_put_target = action;
}

GameSim::GarbagePhase::GarbagePhase(GameSim *sim) : Phase(sim), wait_t(WAIT_T) {
    assert(sim->current_step);
    std::size_t garbage_send;
    auto sim_op = sim->opponent.lock();
    // std::lock_guard lock(sim->field_m);
    garbage_send = sim->field->calcGarbage(std::accumulate(
        sim->current_step->chains.cbegin(), sim->current_step->chains.cend(), 0,
        [](int acc, const Chain &chain) { return acc + chain.score(); }));
    garbage_send -= sim->field->cancelGarbage(garbage_send);
    if (garbage_send > 0) {
        sim->current_step->garbage_send =
            std::make_shared<GarbageGroup>(garbage_send);
        if (!sim->enable_garbage) {
            sim->current_step->garbage_send->fallAll();
            assert(sim->current_step->garbage_send->done());
        }
        if (sim_op && sim->enable_garbage) {
            // std::lock_guard lock(sim_op->field_m);
            if (sim_op->current_step) {
                sim_op->current_step->garbage_recv.push_back(
                    sim->current_step->garbage_send);
            }
            sim_op->field->addGarbage(sim->current_step->garbage_send);
        }
    }
    // std::lock_guard lock(sim->field_m);
    if (sim->field->getGarbageNumTotal() == 0) {
        wait_t = 0;
    } else {
        sim->field->putGarbage(&sim->current_step->garbage_fell_pos);
    }
}
std::unique_ptr<GameSim::Phase> GameSim::GarbagePhase::step() {
    wait_t--;
    if (wait_t < 0) {
        return std::make_unique<GameSim::FreePhase>(sim);
    }
    return nullptr;
}

GameSim::FreePhase::FreePhase(GameSim *sim) : Phase(sim), put_t(PUT_T) {
    // std::lock_guard lock(sim->field_m);
    auto sim_op = sim->opponent.lock();

    // 前ターンのデータ残り
    if (sim->current_step) {
        sim->current_step->field_after = sim->field;
        if (sim_op) {
            sim->current_step->op_field_after = sim_op->field;
        }
    }

    sim->rot_fail_count = 0;
    sim->is_over = sim->field->isGameOver();

    sim->prev_step = sim->current_step;
    sim->current_step = std::make_shared<StepResult>(*sim->field);
    if(sim->prev_step){
        sim->prev_step->next_ = sim->current_step;
    }
    if (sim_op) {
        sim->current_step->op_field_before = sim_op->field;
    }

    sim->step_count++;
}

std::unique_ptr<GameSim::Phase> GameSim::FreePhase::step() {
    // bool go_fall = false;
    // std::lock_guard lock(sim->field_m);
    auto current_pair = sim->field->getNext(0);
    current_pair.y -= FALL_SPEED / 60;
    auto [yb, yt] = sim->field->getNextHeight(current_pair);
    if (yb < current_pair.bottomY() && yt < current_pair.topY()) {
        put_t = PUT_T;
        sim->field->updateNext(current_pair);
    } else {
        put_t--;
        current_pair.y = yb;
        if (yt > current_pair.topY()) {
            current_pair.y += yt - current_pair.topY();
        }
        sim->field->updateNext(current_pair);
        if (put_t < 0) {
            sim->soft_put_target = std::nullopt;
            sim->field->putNext();
            // go_fall = true;
            return std::make_unique<GameSim::FallPhase>(sim);
        }
    }
    // if (go_fall) {
    // } else {
    return nullptr;
    // }
}

GameSim::FallPhase::FallPhase(GameSim *sim)
    : Phase(sim), current_chain(0), fall_wait_t(0), display_field(*sim->field) {
    // std::shared_lock lock(sim->field_m);
    if (sim->field->fall()) {
        fall_wait_t = FALL_T;
    }
    assert(sim->current_step);
    sim->current_step->chains = sim->field->deleteChainRecurse();
    chain_t.assign(sim->current_step->chains.size(), CHAIN_T + FALL_T);
    display_field.fall();
}
std::unique_ptr<GameSim::Phase> GameSim::FallPhase::step() {
    if (fall_wait_t > 0) {
        fall_wait_t--;
    } else {
        assert(sim->current_step);
        if (current_chain >= sim->current_step->chains.size()) {
            return std::make_unique<GameSim::GarbagePhase>(sim);
        }
        if (chain_t.at(current_chain) == FALL_T + CHAIN_T) {
            display_field.deleteChain(current_chain + 1);
        }
        if (chain_t.at(current_chain) == FALL_T) {
            display_field.fall();
        }
        if (--chain_t.at(current_chain) <= 0) {
            current_chain++;
        }
    }
    return nullptr;
}

} // namespace PUMILA_NS
