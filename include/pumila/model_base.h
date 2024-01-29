#pragma once
#include "game.h"
#include <BS_thread_pool.hpp>
#include <random>
#include <utility>

namespace pumila {
class Pumila {
  protected:
    inline static BS::thread_pool pool;
    inline static std::random_device seed;
    inline static std::mt19937 rnd{seed()};

  public:
    Pumila() = default;
    virtual ~Pumila() {}

    /*!
     * \brief 次の手を取得する
     * \return 0〜21 (actionsに対応)
     *
     */
    virtual int getAction(std::shared_ptr<GameSim> sim) = 0;

    /*!
     * \brief 次の手を取得する (学習用)
     * \return first: 0〜21 (actionsに対応)
     * second: learnResultに渡すid
     *
     */
    virtual std::pair<int, int> getLearnAction(std::shared_ptr<GameSim> sim) {
        return std::make_pair(getAction(sim), 0);
    }

    /*!
     * \brief getLearnActionで得た手を実行した結果を渡し学習させる
     *
     */
    virtual double learnResult(int id, std::shared_ptr<GameSim> sim_after) {
        return 0;
    }
};
} // namespace pumila
