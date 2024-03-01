#include <pumila/game.h>
#include <pumila/model_base.h>
#include <pumila/field.h>
#include <iostream>
#include <cassert>

namespace PUMILA_NS {
GameSim::GameSim(std::shared_ptr<Pumila> model, const std::string &name,
                 typename std::mt19937::result_type seed, bool enable_garbage)
    : model_action_thread(std::nullopt), running(true),
      enable_garbage(enable_garbage), opponent(), field(std::nullopt),
      model(model), name(model && name.empty() ? model->name() : name),
      phase(nullptr) {
    if (model) {
        model_action_thread = std::make_optional<std::thread>([this] {
            while (running.load()) {
                while (running.load() && !isFreePhase()) {
                    std::this_thread::yield();
                }
                std::optional<FieldState2> field_copy;
                {
                    std::shared_lock lock(field_m);
                    if (field) {
                        field_copy = *field;
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
    }
    reset(seed);
}

void GameSim::reset(typename std::mt19937::result_type seed) {
    std::lock_guard lock(step_m);
    {
        std::lock_guard lock2(field_m);
        field = std::make_optional<FieldState2>(seed);
    }
    phase = std::make_unique<GameSim::GarbagePhase>(this); // nextが補充される
}

void GameSim::stopAction() {
    running.store(false);
    if (model_action_thread) {
        model_action_thread->join();
        model_action_thread = std::nullopt;
    }
}

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

std::shared_ptr<FieldState> GameSim::field1() {
    std::shared_lock lock(field_m);
    return std::make_shared<FieldState>(*field);
}

void GameSim::movePair(int dx) {
    std::lock_guard lock(step_m);
    if (isFreePhase()) {
        std::lock_guard lock(field_m);
        auto pp = field->next().get();
        if (FieldState2::inRange(pp.bottomX() + dx) &&
            FieldState2::inRange(pp.topX() + dx) &&
            (pp.bottomY() > 12 ||
             field->field().get(pp.bottomX() + dx, pp.bottomY()) ==
                 Puyo::none) &&
            (pp.topY() > 12 ||
             field->field().get(pp.topX() + dx, pp.topY()) == Puyo::none)) {
            pp.x += dx;
            field->next().update(pp);
        }
    }
}
void GameSim::rotPair(int r) {
    std::lock_guard lock(step_m);
    if (isFreePhase() && rot_fail_count < ROT_FAIL_COUNT) {
        std::lock_guard lock(field_m);
        PuyoPair new_pp = field->next().get();
        if (r == rot_fail) {
            r *= 2;
        }
        rot_fail = 0;
        new_pp.rotate(r);
        if (!field->checkNextCollision(new_pp)) {
            field->next().update(new_pp);
            return;
        }
        PuyoPair new_pp2 = new_pp;
        rot_fail_count++;
        new_pp2.y = new_pp.y + 1;
        if (!field->checkNextCollision(new_pp2)) {
            field->next().update(new_pp2);
            return;
        }
        new_pp2.y = new_pp.y;
        new_pp2.x = new_pp.x + 1;
        if (!field->checkNextCollision(new_pp2)) {
            field->next().update(new_pp2);
            return;
        }
        new_pp2.y = new_pp.y;
        new_pp2.x = new_pp.x - 1;
        if (!field->checkNextCollision(new_pp2)) {
            field->next().update(new_pp2);
            return;
        }
        rot_fail = r;
    }
}
void GameSim::quickDrop() {
    std::lock_guard lock(step_m);
    if (isFreePhase()) {
        std::lock_guard lock(field_m);
        PuyoPair pp = field->next().get();
        pp.y = -1;
        field->next().update(pp);
        auto f_phase = dynamic_cast<FreePhase *>(phase.get());
        f_phase->put_t = 0;
    }
}
void GameSim::softDrop() {
    std::lock_guard lock(step_m);
    if (isFreePhase()) {
        std::lock_guard lock(field_m);
        PuyoPair pp = field->next().get();
        pp.y -= FreePhase::SOFT_SPEED / 60;
        field->next().update(pp);
        auto f_phase = dynamic_cast<FreePhase *>(phase.get());
        f_phase->put_t -= 10;
    }
}

void GameSim::step() {
    std::lock_guard lock(step_m);
    if (soft_put_target) {
        PuyoPair pp;
        {
            std::shared_lock lock(field_m);
            pp = field->next().get();
        }
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
    std::lock_guard lock(step_m);
    if (isFreePhase()) {
        {
            std::lock_guard lock(field_m);
            PuyoPair pp = field->next().get();
            field->next().update({pp, action});
        }
        quickDrop();
        step();
    }
}
void GameSim::softPut(const Action &action) {
    std::lock_guard lock(step_m);
    soft_put_target = action;
}

bool GameSim::isFreePhase() {
    std::lock_guard lock(step_m);
    return phase && phase->get() == Phase::free;
}

GameSim::GarbagePhase::GarbagePhase(GameSim *sim)
    : Phase(sim), wait_t(WAIT_T), garbage(), garbage_num(0),
      field_prev(*sim->field) {
    int garbage_send;
    {
        std::lock_guard lock(sim->field_m);
        garbage_send = sim->field->garbage().send();
    }
    auto opponent_sim = sim->opponent.lock();
    if (opponent_sim && sim->enable_garbage) {
        std::lock_guard lock(opponent_sim->field_m);
        opponent_sim->field->garbage().addGarbage(garbage_send);
    }
    {
        std::lock_guard lock(sim->field_m);
        if (sim->field->garbage().getReady() == 0) {
            wait_t = 0;
        } else {
            sim->field->putGarbage(&garbage, &garbage_num);
        }
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
    std::lock_guard lock(sim->field_m);
    sim->rot_fail_count = 0;
    sim->is_over = sim->field->isGameOver();
}

std::unique_ptr<GameSim::Phase> GameSim::FreePhase::step() {
    bool go_fall = false;
    {
        std::lock_guard lock(sim->field_m);
        auto current_pair = sim->field->next().get();
        current_pair.y -= FALL_SPEED / 60;
        auto [yb, yt] = sim->field->getNextHeight(current_pair);
        if (yb < current_pair.bottomY() && yt < current_pair.topY()) {
            put_t = PUT_T;
            sim->field->next().update(current_pair);
        } else {
            put_t--;
            current_pair.y = yb;
            if (yt > current_pair.topY()) {
                current_pair.y += yt - current_pair.topY();
            }
            sim->field->next().update(current_pair);
            if (put_t < 0) {
                sim->soft_put_target = std::nullopt;
                sim->field->putNext(current_pair);
                go_fall = true;
            }
        }
    }
    if (go_fall) {
        return std::make_unique<GameSim::FallPhase>(sim);
    } else {
        return nullptr;
    }
}

GameSim::FallPhase::FallPhase(GameSim *sim)
    : Phase(sim), current_chain(nullptr), fall_wait_t(0),
      display_field(*sim->field) {
    std::shared_lock lock(sim->field_m);
    if (sim->field->fall()) {
        fall_wait_t = Chain::FALL_T;
    }
    sim->field->deleteChainRecurse();
}
std::unique_ptr<GameSim::Phase> GameSim::FallPhase::step() {
    if (fall_wait_t > 0) {
        fall_wait_t--;
    } else {
        auto current_chain_new = sim->field->currentStep().step();
        if (current_chain_new == nullptr) {
            assert(*sim->field == display_field);
            return std::make_unique<GameSim::GarbagePhase>(sim);
        } else if (current_chain_new != current_chain) {
            current_chain = current_chain_new;
            display_field.deleteChain();
        }
        if (current_chain->wait_time == Chain::FALL_T) {
            display_field.fall();
        }
    }
    return nullptr;
}

} // namespace PUMILA_NS
