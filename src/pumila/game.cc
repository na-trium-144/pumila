#include <pumila/game.h>

namespace pumila {
GameSim::GameSim() : field(), last_chain(), seed(), rnd(seed()) {
    // todo: 最初のツモは完全ランダムではなかった気がする
    current_pair.bottom = randomPuyo();
    current_pair.top = randomPuyo();
    next_pair.bottom = randomPuyo();
    next_pair.top = randomPuyo();
    next2_pair.bottom = randomPuyo();
    next2_pair.top = randomPuyo();
    initPair();
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
void GameSim::nextPair() {
    current_pair = next_pair;
    next_pair = next2_pair;
    next2_pair.bottom = randomPuyo();
    next2_pair.top = randomPuyo();
    initPair();
}
void GameSim::initPair() {
    current_pair_y = prev_pair_y = 12;
    current_pair_x = 2;
    current_pair_rot = static_cast<int>(Rotation::vertical);
    current_pair_wait_t = PAIR_WAIT_T;
}
void GameSim::movePair(int dx) {
    if (current_pair_x + dx <= FieldState::WIDTH - 1 &&
        current_pair_x + dx >= 0) {
        current_pair_x += dx;
    }
}
void GameSim::rotPair(int r) {
    current_pair_rot = ((current_pair_rot + r) % 4 + 4) % 4;
    if (current_pair_rot == static_cast<int>(Rotation::horizontal_left) &&
        current_pair_x < 1) {
        current_pair_x = 1;
    }
    if (current_pair_rot == static_cast<int>(Rotation::horizontal_right) &&
        current_pair_x >= FieldState::WIDTH - 2) {
        current_pair_x = FieldState::WIDTH - 2;
    }
}
void GameSim::quickDrop() {
    current_pair_y -= 12;
    current_pair_wait_t = 0;
}
void GameSim::step(double ms, bool soft_drop) {
    switch (phase) {
    case Phase::free: {
        double new_pair_y = current_pair_y - (soft_drop ? 20 : 0.3) * ms / 1000;
        bool touched = false;
        for (int y = static_cast<int>(prev_pair_y); y >= new_pair_y; y--) {
            int bottom_y =
                current_pair_rot == static_cast<int>(Rotation::vertical_inverse)
                    ? y - 2
                    : y - 1;
            if (bottom_y < 0 ||
                field.get(current_pair_x, bottom_y) != Puyo::none) {
                touched = true;
                current_pair_wait_t -= (soft_drop ? ms * 10 : ms);
                if (current_pair_wait_t <= 0) {
                    field = field.put(current_pair,
                                      static_cast<Rotation>(current_pair_rot),
                                      current_pair_x);
                    phase = Phase::fall;
                    phase_wait_t = CHAIN_FALL_T;
                    current_chain = 0;
                    field = field.fall();
                    break;
                }
            }
        }
        if (!touched) {
            current_pair_wait_t = PAIR_WAIT_T;
            current_pair_y = prev_pair_y = new_pair_y;
        }
        break;
    }
    case Phase::fall: {
        phase_wait_t -= ms;
        if (phase_wait_t < 0) {
            Chain chain = field.findChain();
            if (chain.connections.empty()) {
                phase = Phase::free;
                nextPair();
                break;
            } else {
                phase = Phase::chain;
                phase_wait_t = CHAIN_ERASE_T;
                last_chain = chain;
                field = last_chain.state_after;
                score += last_chain.score(++current_chain);
                break;
            }
        }
        break;
    }
    case Phase::chain: {
        phase_wait_t -= ms;
        if (phase_wait_t < 0) {
            phase = Phase::fall;
            phase_wait_t = CHAIN_FALL_T;
            field = field.fall();
            break;
        }
        break;
    }
    }
}

} // namespace pumila
