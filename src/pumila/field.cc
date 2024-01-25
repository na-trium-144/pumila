#include <pumila/field.h>
#include <pumila/chain.h>
#include <algorithm>

namespace pumila {

int PuyoPair::topX() const {
    switch (rot) {
    case Rotation::vertical:
    case Rotation::vertical_inverse:
        return x;
    case Rotation::horizontal_left:
        return x - 1;
    case Rotation::horizontal_right:
        return x + 1;
    }
    return 0;
}
double PuyoPair::topY() const {
    switch (rot) {
    case Rotation::vertical:
        return y + 1;
    case Rotation::vertical_inverse:
        return y - 1;
    case Rotation::horizontal_left:
    case Rotation::horizontal_right:
        return y;
    }
    return 0;
}
void PuyoPair::rotate(int right) {
    rot = static_cast<Rotation>(((static_cast<int>(rot) + right) % 4 + 4) % 4);
}

int FieldState::deleteConnection(std::size_t x, std::size_t y) {
    Puyo here = get(x, y);
    if (here == Puyo::none) {
        return 0;
    } else {
        int c = 1;
        put(x, y, Puyo::none);
        if (inRange(x + 1, y) && get(x + 1, y) == here) {
            c += deleteConnection(x + 1, y);
        }
        if (inRange(x - 1, y) && get(x - 1, y) == here) {
            c += deleteConnection(x - 1, y);
        }
        if (inRange(x, y + 1) && get(x, y + 1) == here) {
            c += deleteConnection(x, y + 1);
        }
        if (inRange(x, y - 1) && get(x, y - 1) == here) {
            c += deleteConnection(x, y - 1);
        }
        return c;
    }
}

std::pair<std::size_t, std::size_t> FieldState::putTargetY(const PuyoPair &pp,
                                                           bool fall) const {
    std::size_t yb = putTargetY(pp.bottomX()), yt = putTargetY(pp.topX());
    if (!fall) {
        yb = yt = yb > yt ? yb : yt;
    }
    if (pp.rot == PuyoPair::Rotation::vertical) {
        yt++;
    }
    if (pp.rot == PuyoPair::Rotation::vertical_inverse) {
        yb++;
    }
    return std::make_pair(yb, yt);
}
std::size_t FieldState::putTargetY(std::size_t x) const {
    std::size_t y = HEIGHT - 1;
    for (; y >= 1; y--) {
        if (get(x, y - 1) != Puyo::none) {
            break;
        }
    }
    return y;
}

void FieldState::put(const PuyoPair &pp) {
    auto [yb, yt] = putTargetY(pp, true);
    put(pp.topX(), yt, pp.top);
    put(pp.bottomX(), yb, pp.bottom);
}

Chain FieldState::deleteChain(int chain_num) {
    Chain chain(chain_num);
    FieldState state_tmp = *this;
    for (std::size_t y = 0; y < HEIGHT; y++) {
        for (std::size_t x = 0; x < WIDTH; x++) {
            int connection = state_tmp.deleteConnection(x, y);
            if (connection >= 4) {
                deleteConnection(x, y); // thisの盤面にも反映
                chain.connections.emplace_back(get(x, y), connection);
            }
        }
    }
    return chain;
}
bool FieldState::fall() {
    bool has_fall = false;
    for (std::size_t x = 0; x < WIDTH; x++) {
        for (std::size_t y = 0; y < HEIGHT - 1; y++) {
            if (get(x, y) == Puyo::none) {
                for (std::size_t dy = 1; y + dy < HEIGHT - 1; dy++) {
                    if (get(x, y + dy) != Puyo::none) {
                        put(x, y, get(x, y + dy));
                        put(x, y + dy, Puyo::none);
                        has_fall = true;
                        break;
                    }
                }
            }
        }
    }
    return has_fall;
}

} // namespace pumila
