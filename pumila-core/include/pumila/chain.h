#pragma once
#include "def.h"
#include "action.h"
#include <vector>
#include <utility>

namespace PUMILA_NS {
struct PuyoConnection {
    std::vector<std::pair<std::size_t, std::size_t>> colored, garbage;
    PuyoConnection() : colored(), garbage() {}
};
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
     */
    std::vector<std::pair<Puyo, int>> connections;
    int chain_num;
    explicit Chain(int chain_num)
        : connections(), chain_num(chain_num){}
    void push_connection(Puyo p, int n);
    bool isEmpty() const { return connections.empty(); }
    PUMILA_DLL int connectionNum() const;
    int chainBonus() const { return chainBonus(chain_num); }
    PUMILA_DLL static int chainBonus(int chain_num);
    PUMILA_DLL int connectionBonus() const;
    PUMILA_DLL int colorBonus() const;
    PUMILA_DLL int scoreA() const;
    PUMILA_DLL int scoreB() const;
    int score() const { return scoreA() * scoreB(); };

    PUMILA_DLL bool operator==(const Chain &other) const;
    bool operator!=(const Chain &other) const { return !(*this == other); }
};

} // namespace PUMILA_NS
