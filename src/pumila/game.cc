#include <pumila/game.h>
#include <pumila/chain.h>
#include <algorithm>

namespace pumila {

int GameState::findConnect(std::size_t x, std::size_t y) {
    int c = 1;
    put(x, y, Puyo::none);
    if (x + 1 < WIDTH && get(x + 1, y) == get(x, y)) {
        c += findConnect(x + 1, y);
    }
    if (x >= 1 && get(x - 1, y) == get(x, y)) {
        c += findConnect(x - 1, y);
    }
    if (y + 1 < HEIGHT && get(x, y + 1) == get(x, y)) {
        c += findConnect(x, y + 1);
    }
    if (y >= 1 && get(x, y - 1) == get(x, y)) {
        c += findConnect(x, y - 1);
    }
    return c;
}
GameState GameState::put(const PuyoPair &pp, Rotation rot,
                         std::size_t x) const {
    GameState new_state = *this;
    std::size_t y = 0;
    for (; y < HEIGHT; y++) {
        if (new_state.get(x, y) == Puyo::none) {
            break;
        }
    }
    switch (rot) {
    case Rotation::vertical:
        new_state.put(y, x, pp.bottom);
        new_state.put(y + 1, x, pp.top);
        break;
    case Rotation::vertical_inverse:
        new_state.put(y + 1, x, pp.bottom);
        new_state.put(y, x, pp.top);
        break;
    case Rotation::horizontal_right:
        new_state.put(y, x, pp.bottom);
        new_state.put(y, x + 1, pp.top);
        break;
    case Rotation::horizontal_left:
        new_state.put(y, x, pp.bottom);
        new_state.put(y, x - 1, pp.top);
        break;
    }
    return new_state;
}

Chain GameState::findChain() const {
    Chain chain = {*this, {}};
    GameState state_tmp = *this;
    for (std::size_t y = 0; y < HEIGHT; y++) {
        for (std::size_t x = 0; x < WIDTH; x++) {
            int connection = state_tmp.findConnect(x, y);
            if (connection >= 4) {
                chain.state_after.findConnect(x, y);
                chain.connections.emplace_back(get(x, y), connection);
            }
        }
    }
    return chain;
}
void GameState::fall() {
    for (std::size_t y = 0; y < HEIGHT - 1; y++) {
        for (std::size_t x = 0; x < WIDTH; x++) {
            if (get(x, y) == Puyo::none) {
                put(x, y, get(x, y + 1));
                put(x, y + 1, Puyo::none);
            }
        }
    }
}

} // namespace pumila
