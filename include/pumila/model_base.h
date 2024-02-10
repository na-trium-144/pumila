#pragma once
#include "def.h"
#include "game.h"
#include "field.h"
#include <BS_thread_pool.hpp>
#include <random>
#include <memory>
#include <string>
#include <iostream>

namespace PUMILA_NS {
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

  public:
    Pumila() = default;
    virtual ~Pumila() = default;

    Pumila(const Pumila &) = delete;
    Pumila(Pumila &&) = delete;

    /*!
     * \brief ファイル名に使う
     * 空文字列の場合loadもsaveもされない
     */
    virtual std::string name() const { return ""; };
    /*!
     * \brief ファイルを開き load() を呼ぶ
     */
    PUMILA_DLL void loadFile();
    PUMILA_DLL void loadFile(std::string file_name);
    /*!
     * \brief ファイルを開き save() を呼ぶ
     */
    PUMILA_DLL void saveFile();
    PUMILA_DLL void saveFile(std::string file_name);

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
} // namespace PUMILA_NS
