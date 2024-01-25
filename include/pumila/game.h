#pragma once
#include "field.h"
#include "chain.h"
#include <random>

namespace pumila {
/*!
 * \brief fieldに加えてネクスト、落下時間、スコアなども管理する(1プレイヤー分)
 *
 */
class GameSim {
    std::random_device seed;
    std::mt19937 rnd;
    Puyo randomPuyo();

    void initPair();
    void nextPair();

  public:
    FieldState field;
    PuyoPair current_pair, next_pair, next2_pair;
    int score = 0;
    Chain last_chain;

    // todo: structにわけるなりして管理する
    double current_pair_y, prev_pair_y;
    int current_pair_x;
    int current_pair_rot;
    double current_pair_wait_t;
    static constexpr double PAIR_WAIT_T = 2000;
    static constexpr double CHAIN_ERASE_T = 500;
    static constexpr double CHAIN_FALL_T = 500;

    enum class Phase{
        free,
        fall,
        chain,
    } phase = Phase::free;
    int current_chain = 0;
    double phase_wait_t = 0;

    GameSim();

    void movePair(int dx);
    void rotPair(int r);
    void quickDrop();

    void step(double ms = 16, bool soft_drop = false);

};

} // namespace pumila
