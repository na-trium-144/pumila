#include <pumila/field2.h>
#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <cmath>
#include <iostream>

namespace PUMILA_NS {
void FieldState2::Field::set(std::size_t x, std::size_t y, Puyo p) {
    if (inRange(x, y)) {
        if (p == Puyo::none && field.at(y).at(x) != Puyo::none) {
            puyo_num--;
        } else if (p != Puyo::none && field.at(y).at(x) == Puyo::none) {
            puyo_num++;
        }
        field.at(y).at(x) = p;
        updated.at(y).at(x) = true;
    }
}

Puyo FieldState2::Field::get(std::size_t x, std::size_t y) const {
    assert(inRange(x, y)); // debug時のみ
    if (!inRange(x, y)) {
        std::cerr << "out of range in FieldState2::Field::get(x = " << x
                  << ", y = " << y << ")" << std::endl;
        return Puyo::none;
    }
    return field.at(y).at(x);
}
bool FieldState2::Field::getUpdated(std::size_t x, std::size_t y) const {
    assert(inRange(x, y)); // debug時のみ
    if (!inRange(x, y)) {
        std::cerr << "out of range in FieldState2::Field::getUpdated(x = " << x
                  << ", y = " << y << ")" << std::endl;
        return false;
    }
    return updated.at(y).at(x);
}

PuyoPair FieldState2::NextList::get() const {
    if (next_num == 0) {
        throw std::out_of_range("field.next is not available");
    }
    return next[0];
}
void FieldState2::NextList::update(const PuyoPair &pp) {
    if (next_num == 0) {
        throw std::out_of_range("field.next is not available");
    }
    next[0] = pp;
}
void FieldState2::NextList::pop() {
    if (next_num == 0) {
        throw std::out_of_range("field.next is not available");
    }
    for (std::size_t i = 1; i < next_num && i < next.size(); i++) {
        next[i - 1] = next[i];
    }
    next_num--;
}
void FieldState2::NextList::push(const PuyoPair &pp) {
    if (next_num >= next.size()) {
        throw std::out_of_range("field.next is full");
    }
    next[next_num++] = pp;
}


void FieldState2::GarbageInfo::pushScore(int score_add) {
    garbage_score += score_add;
    int garbage_add = garbage_score / rate;
    garbage_current += garbage_add;
    garbage_score -= garbage_add * rate;
}
int FieldState2::GarbageInfo::send() {
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


PuyoConnection FieldState2::deleteConnection(std::size_t x, std::size_t y) {
    PuyoConnection deleted;
    deleteConnectionImpl(x, y, deleted);
    return deleted;
}
void FieldState2::deleteConnectionImpl(std::size_t x, std::size_t y,
                                       PuyoConnection &deleted) {
    if (!inRange(x, y)) {
        std::cerr << "out of range in FieldState::deleteConnection(x = " << x
                  << ", y = " << y << ")";
        return;
    }
    Puyo here = field_.get(x, y);
    if (here == Puyo::none) {
        return;
    }
    field_.set(x, y, Puyo::none);
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
            if (inRange(nx, ny) && (field_.get(nx, ny) == here ||
                                    field_.get(nx, ny) == Puyo::garbage)) {
                deleteConnectionImpl(nx, ny, deleted);
            }
        }
    }
}

std::pair<std::size_t, std::size_t>
FieldState2::getNextHeight(const Action &action) const {
    std::size_t yb = getHeight(action.bottomX()), yt = getHeight(action.topX());
    if (action.rot == PuyoPair::Rotation::vertical) {
        yt++;
    }
    if (action.rot == PuyoPair::Rotation::vertical_inverse) {
        yb++;
    }
    return std::make_pair(yb, yt);
}
std::size_t FieldState2::getHeight(std::size_t x) const {
    std::size_t y = HEIGHT;
    for (; y >= 1; y--) {
        if (field_.get(x, y - 1) != Puyo::none) {
            break;
        }
    }
    return y;
}

void FieldState2::putNext(const Action &action) {
    PuyoPair pp{next_.get(), action};
    auto [yb, yt] = getNextHeight(action);
    prev_puyo_num_ = field_.puyoNum();
    field_.clearUpdated();
    field_.set(pp.topX(), yt, pp.top);
    field_.set(pp.bottomX(), yb, pp.bottom);
    next_.pop();
    if (current_step_.chain_num > 0) {
        last_chain_step_ = current_step_;
    }
    prev_step_ = current_step_;
    current_step_.num++;
    current_step_.chain_num = 0;
    current_step_.chain_score = 0;
}
void FieldState2::putGarbage() {
    std::size_t r = 0;
    std::size_t garbage_num = static_cast<std::size_t>(garbage_.getReady());
    for (; (r + 1) * WIDTH <= garbage_num && r < 5; r++) {
        for (std::size_t x = 0; x < WIDTH; x++) {
            field_.set(x, getHeight(x), Puyo::garbage);
        }
    }
    if (r < 5) {
        std::array<int, WIDTH> target_x = {0, 1, 2, 3, 4, 5};
        std::shuffle(target_x.begin(), target_x.end(), rnd);
        for (std::size_t i = 0; r * WIDTH + i < garbage_num; i++) {
            field_.set(target_x[i], getHeight(target_x[i]), Puyo::garbage);
        }
        garbage_.addGarbage(-1 * static_cast<int>(garbage_num));
    } else {
        garbage_.addGarbage(-1 * static_cast<int>(WIDTH * 5));
    }
}

bool FieldState2::checkNextCollision(const Action &action) const {
    PuyoPair pp{next_.get(), action};
    return (
        (pp.bottomY() < HEIGHT &&
         (!inRange(pp.bottomX(), std::floor(pp.bottomY())) ||
          !inRange(pp.topX(), std::floor(pp.topY())) ||
          field_.get(pp.bottomX(), std::floor(pp.bottomY())) != Puyo::none ||
          field_.get(pp.topX(), std::floor(pp.topY())) != Puyo::none)) ||
        (pp.topY() < HEIGHT &&
         (!inRange(pp.bottomX(), std::ceil(pp.bottomY())) ||
          !inRange(pp.topX(), std::ceil(pp.topY())) ||
          field_.get(pp.bottomX(), std::ceil(pp.bottomY())) != Puyo::none ||
          field_.get(pp.topX(), std::ceil(pp.topY())) != Puyo::none)));
}

Chain FieldState2::deleteChain() {
    int chain_num = current_step_.chain_num + 1;
    Chain chain(chain_num);
    FieldState2 state_tmp = *this;
    for (std::size_t y = 0; y < HEIGHT; y++) {
        for (std::size_t x = 0; x < WIDTH; x++) {
            if (field_.getUpdated(x, y)) {
                auto connection = state_tmp.deleteConnection(x, y);
                if (connection.colored.size() >= 4) {
                    chain.push_connection(field_.get(x, y),
                                          connection.colored.size());
                    deleteConnection(x, y); // thisの盤面にも反映
                }
            }
        }
    }
    if (!chain.isEmpty()) {
        int chain_score = chain.score();
        current_step_.chain_num = chain_num;
        current_step_.chain_score += chain_score;
        total_score_ += chain_score;
        garbage_.pushScore(chain_score);
    }
    return chain;
}
bool FieldState2::fall() {
    bool has_fall = false;
    for (std::size_t x = 0; x < WIDTH; x++) {
        for (std::size_t y = 0; y < HEIGHT; y++) {
            if (field_.get(x, y) == Puyo::none) {
                for (std::size_t dy = 1; y + dy < HEIGHT; dy++) {
                    if (field_.get(x, y + dy) != Puyo::none) {
                        field_.set(x, y, field_.get(x, y + dy));
                        field_.set(x, y + dy, Puyo::none);
                        has_fall = true;
                        break;
                    }
                }
            }
        }
    }
    return has_fall;
}

std::array<std::array<int, FieldState2::WIDTH>, FieldState2::HEIGHT>
FieldState2::calcChainAll() const {
    std::array<std::array<int, WIDTH>, HEIGHT> chain_map = {};
    for (std::size_t y = 0; y < HEIGHT; y++) {
        for (std::size_t x = 0; x < WIDTH; x++) {
            if (field_.get(x, y) == Puyo::none ||
                field_.get(x, y) == Puyo::garbage || chain_map[y][x] != 0) {
                continue;
            }
            FieldState2 state = *this;
            auto first_connection = state.deleteConnection(x, y);
            auto chains = state.deleteChainRecurse();
            for (const auto &pos : first_connection.colored) {
                chain_map[pos.second][pos.first] = chains.size();
            }
        }
    }
    return chain_map;
}

std::vector<Chain> FieldState2::deleteChainRecurse() {
    std::vector<Chain> chains;
    while (true) {
        fall();
        Chain chain = deleteChain();
        if (chain.isEmpty()) {
            break;
        }
        chains.push_back(std::move(chain));
    }
    return chains;
}

} // namespace PUMILA_NS
