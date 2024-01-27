#include <pumila/game.h>
#include <iostream>

namespace pumila {
GameSim::GameSim() : field(), seed(), rnd(seed()), phase(nullptr) {
    // todo: 最初のツモは完全ランダムではなかった気がする
    next_pair = {randomPuyo(), randomPuyo()};
    next2_pair = {randomPuyo(), randomPuyo()};
    phase = std::make_unique<GameSim::FreePhase>(this);
}
Puyo GameSim::randomPuyo() {
    switch (static_cast<int>(static_cast<double>(rnd() - rnd.min()) /
                             (rnd.max() - rnd.min()) * 4)) {
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

PuyoPair GameSim::getCurrentPair() const {
    if (phase->get() == Phase::free) {
        auto f_phase = dynamic_cast<FreePhase *>(phase.get());
        return f_phase->current_pair;
    }
    return PuyoPair{};
}
void GameSim::movePair(int dx) {
    if (phase->get() == Phase::free) {
        auto f_phase = dynamic_cast<FreePhase *>(phase.get());
        auto &pp = f_phase->current_pair;
        if (FieldState::inRange(pp.bottomX() + dx) &&
            FieldState::inRange(pp.topX() + dx) &&
            field.get(pp.bottomX() + dx, pp.bottomY()) == Puyo::none &&
            field.get(pp.topX() + dx, pp.topY()) == Puyo::none) {
            pp.x += dx;
        }
    }
}
void GameSim::rotPair(int r) {
    if (phase->get() == Phase::free) {
        auto f_phase = dynamic_cast<FreePhase *>(phase.get());
        PuyoPair &pp = f_phase->current_pair;
        PuyoPair new_pp = pp;
        new_pp.rotate(r);
        if (new_pp.bottomX() > static_cast<int>(FieldState::WIDTH) - 1 ||
            new_pp.topX() > static_cast<int>(FieldState::WIDTH) - 1) {
            new_pp.x--;
        }
        if (new_pp.bottomX() < 0 || new_pp.topX() < 0) {
            new_pp.x++;
        }
        if (field.get(new_pp.bottomX(), new_pp.bottomY()) == Puyo::none &&
            field.get(new_pp.topX(), new_pp.topY()) == Puyo::none) {
            pp = new_pp;
        }
    }
}
void GameSim::quickDrop() {
    if (phase->get() == Phase::free) {
        auto f_phase = dynamic_cast<FreePhase *>(phase.get());
        PuyoPair &pp = f_phase->current_pair;
        pp.y -= 12;
        f_phase->put_t = 0;
    }
}
void GameSim::softDrop() {
    if (phase->get() == Phase::free) {
        auto f_phase = dynamic_cast<FreePhase *>(phase.get());
        PuyoPair &pp = f_phase->current_pair;
        pp.y -= 20.0 / 60;
        f_phase->put_t -= 10;
    }
}

void GameSim::step() {
    std::unique_ptr<Phase> next_phase = phase->step();
    if (next_phase) {
        phase = std::move(next_phase);
    }
}

GameSim::FreePhase::FreePhase(GameSim *sim) : Phase(sim), put_t(PUT_T) {
    current_pair = sim->next_pair;
    sim->next_pair = sim->next2_pair;
    sim->next2_pair = {sim->randomPuyo(), sim->randomPuyo()};
}

std::unique_ptr<GameSim::Phase> GameSim::FreePhase::step() {
    current_pair.y -= 0.5 / 60;
    auto [yb, yt] = sim->field.putTargetY(current_pair, false);
    auto [yb_f, yt_f] = sim->field.putTargetY(current_pair, true);
    if (yb < current_pair.y) {
        put_t = PUT_T;
    } else {
        put_t--;
        current_pair.y = yb;
        if (put_t < 0) {
            sim->field.put(current_pair);
            sim->current_chain = std::nullopt;
            return std::make_unique<GameSim::FallPhase>(sim);
        }
    }
    return nullptr;
}

GameSim::FallPhase::FallPhase(GameSim *sim) : Phase(sim), wait_t(WAIT_T) {
    if (!sim->field.fall()) {
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
    Chain chain = sim->field.deleteChain(chain_num);
    if (chain.isEmpty()) {
        wait_t = 0;
        sim->current_chain = std::nullopt;
    } else {
        sim->score += chain.score();
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

} // namespace pumila
