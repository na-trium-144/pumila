#include <pumila/field.h>
#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <sstream>
#include <cmath>
#include <iostream>

namespace PUMILA_NS {
Puyo FieldState::get(std::size_t x, std::size_t y) const {
    assert(inRange(x, y)); // debug時のみ
    if (!inRange(x, y)) {
        std::cerr << "out of range in FieldState::get(x = " << x
                  << ", y = " << y << ")";
        return Puyo::none;
    }
    return field.at(y).at(x);
}
PuyoPair FieldState::getNext() const {
    if (next.empty()) {
        throw std::out_of_range("field.next is not available");
    }
    return next[0];
}
void FieldState::updateNext(const PuyoPair &pp) {
    if (next.empty()) {
        throw std::out_of_range("field.next is not available");
    }
    next[0] = pp;
}
void FieldState::popNext() {
    if (next.empty()) {
        throw std::out_of_range("field.next is not available");
    }
    next.pop_front();
}
void FieldState::pushNext(const PuyoPair &pp) { next.push_back(pp); }


void FieldState::addCurrentGarbage(int score_add, int rate) {
    garbage_score += score_add;
    int garbage_add = garbage_score / rate;
    garbage_current += garbage_add;
    garbage_score -= garbage_add * rate;
}
int FieldState::calcGarbageSend() {
    int garbage_diff = garbage_current - garbage_ready;
    garbage_current = 0;
    if (garbage_diff > 0) {
        garbage_ready = 0;
        return garbage_diff;
    } else {
        garbage_ready = -garbage_diff;
        return 0;
    }
}


void FieldState::put(std::size_t x, std::size_t y, Puyo p) {
    if (inRange(x, y)) {
        field.at(y).at(x) = p;
        updated.at(y).at(x) = true;
    } else {
        is_valid = false;
    }
}

FieldState::PuyoConnection FieldState::deleteConnection(std::size_t x,
                                                        std::size_t y) {
    FieldState::PuyoConnection deleted;
    deleteConnection(x, y, deleted);
    return deleted;
}
void FieldState::deleteConnection(std::size_t x, std::size_t y,
                                  FieldState::PuyoConnection &deleted) {
    if (!inRange(x, y)) {
        std::cerr << "out of range in FieldState::deleteConnection(x = " << x
                  << ", y = " << y << ")";
        return;
    }
    Puyo here = get(x, y);
    if (here == Puyo::none) {
        return;
    }
    put(x, y, Puyo::none);
    if (here == Puyo::garbage) {
        deleted.garbage.emplace_back(x, y);
    } else {
        deleted.colored.emplace_back(x, y);
        std::array<std::pair<std::size_t, std::size_t>, 4> near{{
            {x + 1, y},
            {x - 1, y},
            {x, y + 1},
            {x, y - 1},
        }};
        for (const auto &[nx, ny] : near) {
            if (inRange(nx, ny) &&
                (get(nx, ny) == here || get(nx, ny) == Puyo::garbage)) {
                deleteConnection(nx, ny, deleted);
            }
        }
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
    clearUpdateFlag();
    auto [yb, yt] = getHeight(pp, true);
    put(pp.topX(), yt, pp.top);
    put(pp.bottomX(), yb, pp.bottom);
}

bool FieldState::checkCollision(const PuyoPair &pp) {
    return ((pp.bottomY() < HEIGHT &&
             (!inRange(pp.bottomX(), std::floor(pp.bottomY())) ||
              !inRange(pp.topX(), std::floor(pp.topY())) ||
              get(pp.bottomX(), std::floor(pp.bottomY())) != Puyo::none ||
              get(pp.topX(), std::floor(pp.topY())) != Puyo::none)) ||
            (pp.topY() < HEIGHT &&
             (!inRange(pp.bottomX(), std::ceil(pp.bottomY())) ||
              !inRange(pp.topX(), std::ceil(pp.topY())) ||
              get(pp.bottomX(), std::ceil(pp.bottomY())) != Puyo::none ||
              get(pp.topX(), std::ceil(pp.topY())) != Puyo::none)));
}

Chain FieldState::deleteChain(int chain_num) {
    assert(chain_num >= 1);
    Chain chain(chain_num);
    FieldState state_tmp = *this;
    for (std::size_t y = 0; y < HEIGHT; y++) {
        for (std::size_t x = 0; x < WIDTH; x++) {
            if (updated.at(y).at(x)) {
                auto connection = state_tmp.deleteConnection(x, y);
                if (connection.colored.size() >= 4) {
                    chain.connections.emplace_back(get(x, y),
                                                   connection.colored.size());
                    deleteConnection(x, y); // thisの盤面にも反映
                }
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
    checkGameOver();
    return has_fall;
}
bool FieldState::checkGameOver() {
    // ハードコードしている
    if (get(2, 11) != Puyo::none) {
        is_over = true;
    }
    return is_over;
}

std::array<std::array<int, FieldState::WIDTH>, FieldState::HEIGHT>
FieldState::calcChainAll() const {
    std::array<std::array<int, FieldState::WIDTH>, FieldState::HEIGHT>
        chain_map = {};
    for (std::size_t y = 0; y < HEIGHT; y++) {
        for (std::size_t x = 0; x < WIDTH; x++) {
            if (get(x, y) == Puyo::none || get(x, y) == Puyo::garbage ||
                chain_map[y][x] != 0) {
                continue;
            }
            FieldState state = *this;
            auto first_connection = state.deleteConnection(x, y);
            auto chains = state.deleteChainRecurse();
            for (const auto &pos : first_connection.colored) {
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
