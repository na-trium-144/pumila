#include <pumila/field.h>
#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <sstream>
#include <cmath>

namespace PUMILA_NS {
Puyo FieldState::get(std::size_t x, std::size_t y) const {
    if (!inRange(x, y)) {
        std::ostringstream ss;
        ss << "out of range in FieldState::get(x = " << x << ", y = " << y
           << ")";
        throw std::out_of_range(ss.str());
    }
    return field.at(y).at(x);
}

void FieldState::put(std::size_t x, std::size_t y, Puyo p) {
    if (inRange(x, y)) {
        field.at(y).at(x) = p;
    } else {
        is_valid = false;
    }
}

std::vector<std::pair<std::size_t, std::size_t>>
FieldState::deleteConnection(std::size_t x, std::size_t y) {
    std::vector<std::pair<std::size_t, std::size_t>> deleted;
    deleteConnection(x, y, deleted);
    return deleted;
}
void FieldState::deleteConnection(
    std::size_t x, std::size_t y,
    std::vector<std::pair<std::size_t, std::size_t>> &deleted) {
    Puyo here = get(x, y);
    if (here == Puyo::none) {
        return;
    } else {
        deleted.emplace_back(x, y);
        put(x, y, Puyo::none);
        if (inRange(x + 1, y) && get(x + 1, y) == here) {
            deleteConnection(x + 1, y, deleted);
        }
        if (inRange(x - 1, y) && get(x - 1, y) == here) {
            deleteConnection(x - 1, y, deleted);
        }
        if (inRange(x, y + 1) && get(x, y + 1) == here) {
            deleteConnection(x, y + 1, deleted);
        }
        if (inRange(x, y - 1) && get(x, y - 1) == here) {
            deleteConnection(x, y - 1, deleted);
        }
        return;
    }
}

std::pair<std::size_t, std::size_t> FieldState::getHeight(const PuyoPair &pp,
                                                          bool fall) const {
    std::size_t yb = getHeight(pp.bottomX()), yt = getHeight(pp.topX());
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
std::size_t FieldState::getHeight(std::size_t x) const {
    std::size_t y = HEIGHT;
    for (; y >= 1; y--) {
        if (get(x, y - 1) != Puyo::none) {
            break;
        }
    }
    return y;
}

void FieldState::put(const PuyoPair &pp) {
    auto [yb, yt] = getHeight(pp, true);
    put(pp.topX(), yt, pp.top);
    put(pp.bottomX(), yb, pp.bottom);
}

bool FieldState::checkCollision(const PuyoPair &pp) {
    return (!inRange(pp.bottomX(), std::floor(pp.bottomY())) ||
            !inRange(pp.topX(), std::floor(pp.topY())) ||
            get(pp.bottomX(), std::floor(pp.bottomY())) != Puyo::none ||
            get(pp.topX(), std::floor(pp.topY())) != Puyo::none ||
            get(pp.bottomX(), std::ceil(pp.bottomY())) != Puyo::none ||
            get(pp.topX(), std::ceil(pp.topY())) != Puyo::none);
}

Chain FieldState::deleteChain(int chain_num) {
    assert(chain_num >= 1);
    Chain chain(chain_num);
    FieldState state_tmp = *this;
    for (std::size_t y = 0; y < HEIGHT; y++) {
        for (std::size_t x = 0; x < WIDTH; x++) {
            auto connection = state_tmp.deleteConnection(x, y);
            if (connection.size() >= 4) {
                chain.connections.emplace_back(get(x, y), connection.size());
                deleteConnection(x, y); // thisの盤面にも反映
            }
        }
    }
    return chain;
}
bool FieldState::fall() {
    bool has_fall = false;
    for (std::size_t x = 0; x < WIDTH; x++) {
        for (std::size_t y = 0; y < HEIGHT; y++) {
            if (get(x, y) == Puyo::none) {
                for (std::size_t dy = 1; y + dy < HEIGHT; dy++) {
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
    if (get(Action::START_X, Action::START_Y) != Puyo::none) {
        is_over = true;
    }
    return has_fall;
}

std::array<std::array<int, FieldState::WIDTH>, FieldState::HEIGHT>
FieldState::calcChainAll() const {
    std::array<std::array<int, FieldState::WIDTH>, FieldState::HEIGHT>
        chain_map = {};
    for (std::size_t y = 0; y < HEIGHT - 1; y++) {
        for (std::size_t x = 0; x < WIDTH - 1; x++) {
            if (get(x, y) == Puyo::none || chain_map[y][x] != 0) {
                continue;
            }
            FieldState state = *this;
            auto first_connection = state.deleteConnection(x, y);
            auto chains = state.deleteChainRecurse();
            for (const auto &pos : first_connection) {
                chain_map[pos.second][pos.first] = chains.size();
            }
        }
    }
    return chain_map;
}

std::vector<Chain> FieldState::deleteChainRecurse() {
    std::vector<Chain> chains;
    while (true) {
        fall();
        Chain chain = deleteChain(chains.size() + 1);
        if (chain.isEmpty()) {
            break;
        }
        chains.push_back(chain);
    }
    return chains;
}

} // namespace PUMILA_NS
