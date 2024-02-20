#include <pumila/game.h>
#include <pumila/model_base.h>
#include <iostream>

namespace PUMILA_NS {
GameSim::GameSim(std::shared_ptr<Pumila> model, const std::string &name,
                 typename std::mt19937::result_type seed)
    : rnd(seed), model_action_thread(std::nullopt), running(true),
      field(std::make_shared<FieldState>()), current_chain(std::nullopt),
      model(model), name(model && name.empty() ? model->name() : name),
      phase(nullptr) {
    // todo: 最初のツモは完全ランダムではなかった気がする
    {
        std::lock_guard lock(field_m);
        field->pushNext({randomPuyo(), randomPuyo()});
        field->pushNext({randomPuyo(), randomPuyo()});
    }
    phase = std::make_unique<GameSim::FreePhase>(this); // nextが補充される
    if (model) {
        model_action_thread = std::make_optional<std::thread>([this] {
            while (running.load()) {
                while (running.load() && !isFreePhase()) {
                    std::this_thread::yield();
                }
                std::shared_ptr<FieldState> field_copy;
                {
                    std::shared_lock lock(field_m);
                    field_copy = field->copy();
                }
                softPut(actions.at(this->model->getAction(field_copy)));
                while (running.load() && isFreePhase()) {
                    std::this_thread::yield();
                }
            }
        });
    }
}
GameSim::~GameSim() {
    running.store(false);
    if (model_action_thread) {
        model_action_thread->join();
    }
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
        auto pp = field->getNext();
        if (FieldState::inRange(pp.bottomX() + dx) &&
            FieldState::inRange(pp.topX() + dx) &&
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
    std::lock_guard lock(step_m);
    if (isFreePhase()) {
        std::lock_guard lock(field_m);
        PuyoPair new_pp = field->getNext();
        if (r == rot_fail) {
            r *= 2;
        }
        rot_fail = 0;
        new_pp.rotate(r);
        if (!field->checkCollision(new_pp)) {
            field->updateNext(new_pp);
            return;
        }
        PuyoPair new_pp2 = new_pp;
        new_pp2.y = new_pp.y + 1;
        if (!field->checkCollision(new_pp2)) {
            field->updateNext(new_pp2);
            return;
        }
        new_pp2.y = new_pp.y;
        new_pp2.x = new_pp.x + 1;
        if (!field->checkCollision(new_pp2)) {
            field->updateNext(new_pp2);
            return;
        }
        new_pp2.y = new_pp.y;
        new_pp2.x = new_pp.x - 1;
        if (!field->checkCollision(new_pp2)) {
            field->updateNext(new_pp2);
            return;
        }
        rot_fail = r;
    }
}
void GameSim::quickDrop() {
    std::lock_guard lock(step_m);
    if (isFreePhase()) {
        std::lock_guard lock(field_m);
        PuyoPair pp = field->getNext();
        pp.y -= 12;
        field->updateNext(pp);
        auto f_phase = dynamic_cast<FreePhase *>(phase.get());
        f_phase->put_t = 0;
    }
}
void GameSim::softDrop() {
    std::lock_guard lock(step_m);
    if (isFreePhase()) {
        std::lock_guard lock(field_m);
        PuyoPair pp = field->getNext();
        pp.y -= FreePhase::SOFT_SPEED / 60;
        field->updateNext(pp);
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
            pp = field->getNext();
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
            PuyoPair pp = field->getNext();
            field->updateNext({pp, action});
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
    return phase->get() == Phase::free;
}

GameSim::FreePhase::FreePhase(GameSim *sim) : Phase(sim), put_t(PUT_T) {
    std::lock_guard lock(sim->field_m);
    while (sim->field->next_num < 3) {
        sim->field->pushNext({sim->randomPuyo(), sim->randomPuyo()});
    }
}

std::unique_ptr<GameSim::Phase> GameSim::FreePhase::step() {
    bool go_fall = false;
    {
        std::lock_guard lock(sim->field_m);
        auto current_pair = sim->field->getNext();
        current_pair.y -= FALL_SPEED / 60;
        auto [yb, yt] = sim->field->getHeight(current_pair, false);
        // auto [yb_f, yt_f] = sim->field->getHeight(current_pair, true);
        if (yb < current_pair.y) {
            put_t = PUT_T;
            sim->field->updateNext(current_pair);
        } else {
            put_t--;
            current_pair.y = yb;
            sim->field->updateNext(current_pair);
            if (put_t < 0) {
                sim->soft_put_target = std::nullopt;
                sim->field->put(current_pair);
                sim->field->popNext();
                sim->current_chain = std::nullopt;
                if (sim->field->prev_chain_num > 0) {
                    sim->field->last_chain_step_num = sim->field->step_num;
                }
                sim->field->prev_chain_num = 0;
                sim->field->prev_chain_score = 0;
                sim->field->step_num++;
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
    int chain_num = 1;
    if (sim->current_chain) {
        chain_num = sim->current_chain->chain_num + 1;
    }
    std::lock_guard lock(sim->field_m);
    Chain chain = sim->field->deleteChain(chain_num);
    if (chain.isEmpty()) {
        wait_t = 0;
        sim->current_chain = std::nullopt;
    } else {
        sim->field->total_score += chain.score();
        sim->field->prev_chain_score += chain.score();
        sim->field->prev_chain_num++;
        sim->current_chain = std::move(chain);
    }
}

std::unique_ptr<GameSim::Phase> GameSim::ChainPhase::step() {
    wait_t--;
    if (wait_t < 0) {
        if (sim->current_chain) {
            return std::make_unique<GameSim::FallPhase>(sim);
        } else {
            return std::make_unique<GameSim::FreePhase>(sim);
        }
    }
    return nullptr;
}

} // namespace PUMILA_NS
