#pragma once
#include "field.h"
#include "chain.h"
#include <random>
#include <memory>
#include <optional>
#include <atomic>
#include <thread>
#include <mutex>

namespace pumila {
class Pumila;
/*!
 * \brief fieldに加えてネクスト、落下時間、スコアなども管理する(1プレイヤー分)
 *
 * 時間の単位はフレーム (1/60s)
 *
 */
class GameSim {
    std::random_device seed;
    std::mt19937 rnd;
    double getRndD() {
        return static_cast<double>(rnd() - rnd.min()) / (rnd.max() - rnd.min());
    }
    int getRndRange(int num) {
        return static_cast<int>(static_cast<double>(rnd() - rnd.min()) /
                                (rnd.max() - rnd.min()) * num);
    }
    Puyo randomPuyo();

    std::optional<std::thread> model_action_thread;
    std::atomic<bool> running;
    std::optional<Action> soft_put_target = std::nullopt;
    std::recursive_mutex step_m;

  public:
    FieldState field;
    /*!
     * \brief 現在の連鎖情報
     *
     * ChainPhaseを抜けるとリセット
     *
     */
    std::optional<Chain> current_chain;

    std::shared_ptr<Pumila> model;

    int soft_put_interval = 6;
    int soft_put_cnt = 0;

    GameSim() : GameSim(nullptr) {}
    explicit GameSim(std::shared_ptr<Pumila> model);
    GameSim(const GameSim &sim) = delete;
    GameSim(GameSim &&sim) = delete;

    ~GameSim();

    bool hasModel() { return model != nullptr; }

    /*!
     * \brief freePhase時のみぷよを操作する
     *
     * 時間は進まないので適宜step()を呼ぶこと
     *
     */
    void movePair(int dx);
    void rotPair(int r);
    void quickDrop();
    void softDrop();

    /*!
     * \brief 1フレーム時間を進める
     * コンストラクタでmodelをセットしていればそれの返すアクションにしたがってsoftPut()する
     *
     */
    void step();

    /*!
     * \brief ぷよを瞬間移動で置く
     *
     * 時間は進まないので適宜step()を呼ぶこと
     *
     */
    void put(const Action &action);
    /*!
     * \brief ぷよを目標位置まで動かして置くときはここにセットしてstep()を呼ぶ
     */
    void softPut(const Action &action);

    struct Phase {
        GameSim *sim;
        explicit Phase(GameSim *sim) : sim(sim) {}
        virtual ~Phase() {}
        /*!
         * \brief 処理を1周期進める
         *
         * \return 別のフェーズに移行する場合新しいフェーズ、
         * そうでなければnullptr
         *
         */
        virtual std::unique_ptr<Phase> step() = 0;
        enum PhaseEnum {
            free,
            fall,
            chain,
        };
        virtual PhaseEnum get() const = 0;
    };
    std::unique_ptr<Phase> phase;

    bool isFreePhase();

    struct FreePhase final : Phase {
        explicit FreePhase(GameSim *sim);
        PhaseEnum get() const override { return PhaseEnum::free; }
        std::unique_ptr<Phase> step() override;
        static constexpr int PUT_T = 100;
        PuyoPair current_pair;
        int put_t;
    };
    struct FallPhase final : Phase {
        explicit FallPhase(GameSim *sim);
        PhaseEnum get() const override { return PhaseEnum::fall; }
        std::unique_ptr<Phase> step() override;
        static constexpr int WAIT_T = 30;
        int wait_t;
    };
    struct ChainPhase final : Phase {
        explicit ChainPhase(GameSim *sim);
        PhaseEnum get() const override { return PhaseEnum::chain; }
        std::unique_ptr<Phase> step() override;
        static constexpr int WAIT_T = 30;
        int wait_t;
    };
};

} // namespace pumila
