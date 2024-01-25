#include <pumila/field.h>
#include <pumila/chain.h>
#include <algorithm>

namespace pumila {

int FieldState::findConnect(std::size_t x, std::size_t y) {
    Puyo here = get(x, y);
    if (here == Puyo::none) {
        return 0;
    } else {
        int c = 1;
        put(x, y, Puyo::none);
        if (x + 1 < WIDTH && get(x + 1, y) == here) {
            c += findConnect(x + 1, y);
        }
        if (x >= 1 && get(x - 1, y) == here) {
            c += findConnect(x - 1, y);
        }
        if (y + 1 < HEIGHT && get(x, y + 1) == here) {
            c += findConnect(x, y + 1);
        }
        if (y >= 1 && get(x, y - 1) == here) {
            c += findConnect(x, y - 1);
        }
        return c;
    }
}

std::size_t FieldState::putTargetY(Rotation rot, std::size_t x) const{
    std::size_t y = HEIGHT - 1;
    for (; y >= 1; y--) {
        if (get(x, y - 1) != Puyo::none) {
            break;
        }
        if (rot == Rotation::horizontal_left &&
            get(x - 1, y - 1) != Puyo::none) {
            break;
        }
        if (rot == Rotation::horizontal_right &&
            get(x + 1, y - 1) != Puyo::none) {
            break;
        }
    }
    return y;
}
std::size_t FieldState::putTargetY(std::size_t x) const{
    std::size_t y = HEIGHT - 1;
    for (; y >= 1; y--) {
        if (get(x, y - 1) != Puyo::none) {
            break;
        }
    }
    return y;
}
FieldState FieldState::put(const PuyoPair &pp, Rotation rot,
                           std::size_t x) const {
    FieldState new_state = *this;
    std::size_t y = putTargetY(rot, x);
    switch (rot) {
    case Rotation::vertical:
        new_state.put(x, y, pp.bottom);
        new_state.put(x, y + 1, pp.top);
        break;
    case Rotation::vertical_inverse:
        new_state.put(x, y + 1, pp.bottom);
        new_state.put(x, y, pp.top);
        break;
    case Rotation::horizontal_right:
        new_state.put(x, y, pp.bottom);
        new_state.put(x + 1, y, pp.top);
        break;
    case Rotation::horizontal_left:
        new_state.put(x, y, pp.bottom);
        new_state.put(x - 1, y, pp.top);
        break;
    }
    return new_state;
}

Chain FieldState::findChain() const {
    Chain chain = {*this, {}};
    FieldState state_tmp = *this;
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
FieldState FieldState::fall() const {
    FieldState new_field = *this;
    for (std::size_t x = 0; x < WIDTH; x++) {
        for (std::size_t y = 0; y < HEIGHT - 1; y++) {
            if (new_field.get(x, y) == Puyo::none) {
                for (std::size_t dy = 1; y + dy < HEIGHT - 1; dy++) {
                    if (new_field.get(x, y + dy) != Puyo::none) {
                        new_field.put(x, y, new_field.get(x, y + dy));
                        new_field.put(x, y + dy, Puyo::none);
                        break;
                    }
                }
            }
        }
    }
    return new_field;
}

} // namespace pumila
