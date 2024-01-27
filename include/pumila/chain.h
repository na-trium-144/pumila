#pragma once
#include "action.h"
#include <vector>
#include <utility>

namespace pumila {
/*!
 * \brief 1連鎖で消えるぷよの情報
 *
 */
struct Chain {
    /*!
     * \brief ぷよ連結数の情報
     *
     * 例えば赤4連結+青5連結の9個なら{{red, 4}, {blue, 5}}で、
     * connectionNum() = 9
     *
     */
    std::vector<std::pair<Puyo, int>> connections;
    int chain_num;
    Chain(int chain_num) : connections(), chain_num(chain_num) {}
    bool isEmpty() const { return connections.empty(); }
    int connectionNum() const;
    int chainBonus() const;
    int connectionBonus() const;
    int colorBonus() const;
    int scoreA() const;
    int scoreB() const;
    int score() const { return scoreA() * scoreB(); };
};

} // namespace pumila
