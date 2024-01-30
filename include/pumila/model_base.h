#pragma once
#include "game.h"
#include "field.h"
#include <BS_thread_pool.hpp>
#include <random>
#include <memory>

namespace pumila {
class Pumila : public std::enable_shared_from_this<Pumila> {
  protected:
    inline static BS::thread_pool pool;
    inline static std::random_device seed;
    inline static std::mt19937 rnd{seed()};

  public:
    Pumila() = default;
    virtual ~Pumila() = default;

    Pumila(const Pumila &) = delete;
    Pumila(Pumila &&) = delete;

    /*!
     * \brief 次の手を取得する
     * \return 0〜21 (actionsに対応)
     *
     */
    virtual int getAction(const FieldState &field) = 0;
    int getAction(const std::shared_ptr<GameSim> &sim) {
      return getAction(sim->field);
    }
};
} // namespace pumila
