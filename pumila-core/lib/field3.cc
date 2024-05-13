#include "pumila/action.h"
#include <cstddef>
#include <numeric>
#include <pumila/field3.h>

namespace PUMILA_NS {
void FieldState3::set(std::size_t x, std::size_t y, Puyo p) {
    std::lock_guard lock(mtx);
    field.at(y).at(x) = p;
    updated.at(y).at(x) = true;
}

Puyo FieldState3::get(std::size_t x, std::size_t y) const {
    std::lock_guard lock(mtx);
    return field.at(y).at(x);
}
void FieldState3::clearUpdated() {
    std::lock_guard lock(mtx);
    updated = {};
}

Puyo FieldState3::nextColor() {
    std::lock_guard lock(mtx);
    int next_n = static_cast<int>(
        (static_cast<double>(rnd_next()) - rnd_next.min()) /
        (static_cast<double>(rnd_next.max()) - rnd_next.min()) * 4.0);
    switch (next_n) {
    case 0:
        return Puyo::red;
    case 1:
        return Puyo::blue;
    case 2:
        return Puyo::green;
    case 3:
    default:
        return Puyo::yellow;
    }
}

PuyoPair FieldState3::getNext(std::size_t i) const {
    std::lock_guard lock(mtx);
    return next.at(i);
}
void FieldState3::updateNext(const PuyoPair &pp) {
    std::lock_guard lock(mtx);
    next.at(0) = pp;
}
void FieldState3::shiftNext() {
    std::lock_guard lock(mtx);
    for (std::size_t i = 0; i < NextNum - 1; i++) {
        next.at(i) = next.at(i + 1);
    }
    next.at(NextNum - 1) = PuyoPair(nextColor(), nextColor());
}

void FieldState3::addGarbage(const std::shared_ptr<GarbageGroup> &garbage) {
    if (garbage) {
        std::lock_guard lock(mtx);
        garbage_ready.push_back(garbage);
    }
}
std::size_t FieldState3::getGarbageNumTotal() const {
    std::lock_guard lock(mtx);
    return std::accumulate(
        garbage_ready.cbegin(), garbage_ready.cend(),
        static_cast<std::size_t>(0),
        [](std::size_t acc, const std::shared_ptr<GarbageGroup> &g) {
            return acc + g->restGarbageNum();
        });
}

void FieldState3::putNext(const Action &action) {
    std::lock_guard lock(mtx);
    PuyoPair pp{getNext(0), action};
    auto [yb, yt] = getNextHeight(action);
    clearUpdated();
    set(pp.topX(), yt, pp.top);
    set(pp.bottomX(), yb, pp.bottom);
    shiftNext();
}

std::pair<std::size_t, std::size_t>
FieldState3::getNextHeight(const Action &action) const {
    std::lock_guard lock(mtx);
    std::size_t yb = getHeight(action.bottomX()), yt = getHeight(action.topX());
    if (action.rot == PuyoPair::Rotation::vertical) {
        yt++;
    }
    if (action.rot == PuyoPair::Rotation::vertical_inverse) {
        yb++;
    }
    return std::make_pair(yb, yt);
}
std::size_t FieldState3::getHeight(std::size_t x) const {
    std::lock_guard lock(mtx);
    for (std::size_t y = HEIGHT; y >= 1; y--) {
        if (get(x, y - 1) != Puyo::none) {
            return y;
        }
    }
    return 0;
}

void FieldState3::putGarbage(
    std::array<std::pair<std::size_t, std::size_t>, 30> *garbage_list,
    std::size_t *garbage_num) {
    static std::mt19937 rnd_general{std::random_device()()};
    std::lock_guard lock(mtx);
    std::size_t r = 0;
    std::size_t garbage_num_all = getGarbageNumTotal();
    std::size_t garbage_index = 0;
    std::size_t garbage_num_actual;
    for (; (r + 1) * WIDTH <= garbage_num_all && r < 5; r++) {
        for (std::size_t x = 0; x < WIDTH; x++) {
            auto y = getHeight(x);
            set(x, y, Puyo::garbage);
            if (garbage_list) {
                garbage_list->at(garbage_index++) = std::make_pair(x, y);
            }
        }
    }
    if (r < 5) {
        std::array<int, WIDTH> target_x = {0, 1, 2, 3, 4, 5};
        std::shuffle(target_x.begin(), target_x.end(), rnd_general);
        for (std::size_t i = 0; r * WIDTH + i < garbage_num_all; i++) {
            auto y = getHeight(target_x[i]);
            set(target_x[i], y, Puyo::garbage);
            if (garbage_list) {
                garbage_list->at(garbage_index++) =
                    std::make_pair(target_x[i], y);
            }
        }
        garbage_num_actual = garbage_num_all;
    } else {
        garbage_num_actual = WIDTH * 5;
    }
    if (garbage_num) {
        *garbage_num = garbage_num_actual;
    }
    while (garbage_num_actual > 0) {
        auto &gg = garbage_ready.front();
        garbage_num_actual -= gg->fall(garbage_num_actual);
        if (gg->done()) {
            garbage_ready.erase(garbage_ready.begin());
        }
    }
}

std::size_t FieldState3::calcGarbage(int score_add) {
    std::lock_guard lock(mtx);
    garbage_score += score_add;
    std::size_t g = garbage_score / GARBAGE_RATE;
    garbage_score %= GARBAGE_RATE;
    return g;
}
std::size_t FieldState3::cancelGarbage(std::size_t garbage_num) {
    std::lock_guard lock(mtx);
    std::size_t cancelled = 0;
    while (cancelled < garbage_num && !garbage_ready.empty()) {
        auto &gg = garbage_ready.front();
        cancelled += gg->cancel(garbage_num - cancelled);
        if (gg->done()) {
            garbage_ready.erase(garbage_ready.begin());
        }
    }
    return cancelled;
}

bool FieldState3::checkNextCollision(const Action &action) const {
    std::lock_guard lock(mtx);
    PuyoPair pp{getNext(0), action};
    return (
        (std::floor(pp.bottomY()) < HEIGHT && std::floor(pp.topY()) < HEIGHT &&
         (!inRange(pp.bottomX(), std::floor(pp.bottomY())) ||
          !inRange(pp.topX(), std::floor(pp.topY())) ||
          get(pp.bottomX(), std::floor(pp.bottomY())) != Puyo::none ||
          get(pp.topX(), std::floor(pp.topY())) != Puyo::none)) ||
        (std::ceil(pp.bottomY()) < HEIGHT && std::ceil(pp.topY()) < HEIGHT &&
         (!inRange(pp.bottomX(), std::ceil(pp.bottomY())) ||
          !inRange(pp.topX(), std::ceil(pp.topY())) ||
          get(pp.bottomX(), std::ceil(pp.bottomY())) != Puyo::none ||
          get(pp.topX(), std::ceil(pp.topY())) != Puyo::none)));
}

PuyoConnection FieldState3::deleteConnection(std::size_t x, std::size_t y) {
    std::lock_guard lock(mtx);
    PuyoConnection deleted;
    deleteConnectionImpl(x, y, deleted);
    return deleted;
}
void FieldState3::deleteConnectionImpl(std::size_t x, std::size_t y,
                                       PuyoConnection &deleted) {
    std::lock_guard lock(mtx);
    Puyo here = get(x, y);
    if (here == Puyo::none) {
        return;
    }
    set(x, y, Puyo::none);
    if (here == Puyo::garbage) {
        deleted.garbage.emplace_back(x, y);
    } else {
        deleted.colored.emplace_back(x, y);
        const std::array<std::pair<std::size_t, std::size_t>, 4> near{{
            {x + 1, y},
            {x - 1, y},
            {x, y + 1},
            {x, y - 1},
        }};
        for (const auto &[nx, ny] : near) {
            if (inRange(nx, ny) &&
                (get(nx, ny) == here || get(nx, ny) == Puyo::garbage)) {
                deleteConnectionImpl(nx, ny, deleted);
            }
        }
    }
}

Chain FieldState3::deleteChain(std::size_t chain_num) {
    std::lock_guard lock(mtx);
    Chain chain(chain_num);
    FieldState3 state_tmp = *this;
    for (std::size_t y = 0; y < HEIGHT; y++) {
        for (std::size_t x = 0; x < WIDTH; x++) {
            if (updated.at(y).at(x)) {
                auto connection = state_tmp.deleteConnection(x, y);
                if (connection.colored.size() >= 4) {
                    chain.push_connection(get(x, y), connection.colored.size());
                    deleteConnection(x, y); // thisの盤面にも反映
                }
            }
        }
    }
    return chain;
}
bool FieldState3::fall() {
    std::lock_guard lock(mtx);
    bool has_fall = false;
    for (std::size_t x = 0; x < WIDTH; x++) {
        for (std::size_t y = 0; y < HEIGHT; y++) {
            if (get(x, y) == Puyo::none) {
                for (std::size_t dy = 1; y + dy < HEIGHT; dy++) {
                    if (get(x, y + dy) != Puyo::none) {
                        set(x, y, get(x, y + dy));
                        set(x, y + dy, Puyo::none);
                        has_fall = true;
                        break;
                    }
                }
            }
        }
    }
    return has_fall;
}

std::array<std::array<std::size_t, FieldState3::WIDTH>, FieldState3::HEIGHT>
FieldState3::calcChainAll() const {
    std::lock_guard lock(mtx);
    std::array<std::array<std::size_t, WIDTH>, HEIGHT> chain_map = {};
    for (std::size_t y = 0; y < HEIGHT; y++) {
        for (std::size_t x = 0; x < WIDTH; x++) {
            if (get(x, y) == Puyo::none || get(x, y) == Puyo::garbage ||
                chain_map.at(y).at(x) != 0) {
                continue;
            }
            FieldState3 state = *this;
            auto first_connection = state.deleteConnection(x, y);
            auto chains = state.deleteChainRecurse();
            for (const auto &pos : first_connection.colored) {
                chain_map.at(pos.second).at(pos.first) = chains.size();
            }
        }
    }
    return chain_map;
}

std::vector<Chain> FieldState3::deleteChainRecurse() {
    std::lock_guard lock(mtx);
    std::vector<Chain> chains;
    while (true) {
        fall();
        Chain chain = deleteChain(chains.size() + 1);
        if (chain.isEmpty()) {
            break;
        }
        chains.push_back(std::move(chain));
    }
    return chains;
}

} // namespace PUMILA_NS
