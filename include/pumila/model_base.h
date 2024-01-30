#pragma once
#include "game.h"
#include "field.h"
#include <BS_thread_pool.hpp>
#include <random>
#include <memory>
#include <string>
#include <iostream>

namespace pumila {
class Pumila : public std::enable_shared_from_this<Pumila> {
  protected:
    inline static BS::thread_pool pool;
    inline static std::random_device seed;
    inline static std::mt19937 rnd{seed()};
    static double getRndD() {
        return static_cast<double>(rnd() - rnd.min()) / (rnd.max() - rnd.min());
    }
    static int getRndRange(int num) {
        return static_cast<int>(static_cast<double>(rnd() - rnd.min()) /
                                (rnd.max() - rnd.min()) * num);
    }

    virtual void load(std::istream &) {}
    virtual void save(std::ostream &) {}
    /*!
     * \brief ファイル名に使う
     * 空文字列の場合loadもsaveもされない
     */
    virtual std::string name() const { return ""; };

  public:
    Pumila() = default;
    virtual ~Pumila() = default;

    Pumila(const Pumila &) = delete;
    Pumila(Pumila &&) = delete;

    /*!
     * \brief ファイルを開き load() を呼ぶ
     */
    void loadFile();
    /*!
     * \brief ファイルを開き save() を呼ぶ
     */
    void saveFile();

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
