#pragma once
#include "def.h"
#include "action.h"
#include <vector>
#include <utility>

namespace PUMILA_NS {
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
     * 6*13/4=19
     */
    std::array<std::pair<Puyo, int>, 20> connections;
    std::size_t connection_num;
    int chain_num;
    Chain(int chain_num)
        : connections(), connection_num(0), chain_num(chain_num) {}
    void push_connection(Puyo p, int n) ;
    bool isEmpty() const { return connection_num == 0; }
    PUMILA_DLL int connectionNum() const;
    PUMILA_DLL int chainBonus() const;
    PUMILA_DLL int connectionBonus() const;
    PUMILA_DLL int colorBonus() const;
    PUMILA_DLL int scoreA() const;
    PUMILA_DLL int scoreB() const;
    int score() const { return scoreA() * scoreB(); };
};

} // namespace PUMILA_NS
