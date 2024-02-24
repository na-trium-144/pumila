#include <pumila/game.h>
#include <pumila/model_base.h>
#include <pumila/field.h>
#include <iostream>
#include <cassert>

namespace PUMILA_NS {
GameSim::GameSim(std::shared_ptr<Pumila> model, const std::string &name,
                 typename std::mt19937::result_type seed, bool enable_garbage)
    : rnd(seed), model_action_thread(std::nullopt), running(true),
      enable_garbage(enable_garbage), opponent(), field(nullptr),
      current_chain(std::nullopt), model(model),
      name(model && name.empty() ? model->name() : name), phase(nullptr) {
    if (model) {
        model_action_thread = std::make_optional<std::thread>([this] {
            while (running.load()) {
                while (running.load() && !isFreePhase()) {
                    std::this_thread::yield();
                }
                std::shared_ptr<FieldState2> field_copy;
                {
                    std::shared_lock lock(field_m);
                    if (field) {
                        field_copy = std::make_shared<FieldState2>(*field);
                    }
                }
                if (field_copy) {
                    softPut(actions.at(this->model->getAction(field_copy)));
                    while (running.load() && isFreePhase()) {
                        std::this_thread::yield();
                    }
                }
            }
        });
    }
    reset();
}

void GameSim::reset() {
    std::lock_guard lock(step_m);
    {
        std::lock_guard lock2(field_m);
        field = std::make_shared<FieldState2>();
        current_chain = std::nullopt;

        // todo: 最初のツモは完全ランダムではなかった気がする
        field->next().push({randomPuyo(), randomPuyo()});
        field->next().push({randomPuyo(), randomPuyo()});
    }
    phase = std::make_unique<GameSim::GarbagePhase>(this); // nextが補充される
}

GameSim::~GameSim() {
    running.store(false);
    if (model_action_thread) {
        model_action_thread->join();
    }
}

std::shared_ptr<FieldState> GameSim::field1() {
    std::shared_lock lock(field_m);
    return std::make_shared<FieldState>(*field);
}

Puyo GameSim::randomPuyo() {
    switch (getRndRange(4)) {
    case 0:
        return Puyo::red;
    case 1:
        return Puyo::blue;
    case 2:
        return Puyo::green;
    default:
        return Puyo::yellow;
    }
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

GameSim::GarbagePhase::GarbagePhase(GameSim *sim) : Phase(sim), wait_t(WAIT_T) {
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
            sim->field->putGarbage();
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
    while (sim->field->next().size() < 3) {
        sim->field->next().push({sim->randomPuyo(), sim->randomPuyo()});
    }
    sim->rot_fail_count = 0;
    sim->is_over = sim->field->isGameOver();
}

std::unique_ptr<GameSim::Phase> GameSim::FreePhase::step() {
    bool go_fall = false;
    {
        std::lock_guard lock(sim->field_m);
        auto current_pair = sim->field->next().get();
        current_pair.y -= FALL_SPEED / 60;
        auto yb = sim->field->getHeight(current_pair.bottomX());
        auto yt = sim->field->getHeight(current_pair.topX());
        if (yb < current_pair.bottomY() || yt < current_pair.topY()) {
            put_t = PUT_T;
            sim->field->next().update(current_pair);
        } else {
            put_t--;
            current_pair.y = yb;
            sim->field->next().update(current_pair);
            if (put_t < 0) {
                sim->soft_put_target = std::nullopt;
                sim->field->putNext(current_pair);
                sim->current_chain = std::nullopt;
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

GameSim::FallPhase::FallPhase(GameSim *sim) : Phase(sim), wait_t(WAIT_T) {
    std::shared_lock lock(sim->field_m);
    if (!sim->field->fall()) {
        wait_t = 0;
    }
}
std::unique_ptr<GameSim::Phase> GameSim::FallPhase::step() {
    wait_t--;
    if (wait_t < 0) {
        return std::make_unique<GameSim::ChainPhase>(sim);
    }
    return nullptr;
}
GameSim::ChainPhase::ChainPhase(GameSim *sim) : Phase(sim), wait_t(WAIT_T) {
    std::lock_guard lock(sim->field_m);
    Chain chain = sim->field->deleteChain();
    if (chain.isEmpty()) {
        wait_t = 0;
        sim->current_chain = std::nullopt;
    } else {
        sim->current_chain = std::move(chain);
    }
}

std::unique_ptr<GameSim::Phase> GameSim::ChainPhase::step() {
    wait_t--;
    if (wait_t < 0) {
        if (sim->current_chain) {
            assert(sim->field->currentStep().chain_num ==
                   sim->current_chain->chain_num);
            return std::make_unique<GameSim::FallPhase>(sim);
        } else {
            assert(sim->field->currentStep().chain_num == 0);
            return std::make_unique<GameSim::GarbagePhase>(sim);
        }
    }
    return nullptr;
}

} // namespace PUMILA_NS
