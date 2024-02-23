#pragma once
#include "def.h"
#include "field2.h"
#include "chain.h"
#include <random>
#include <memory>
#include <optional>
#include <atomic>
#include <thread>
#include <mutex>
#include <shared_mutex>

namespace PUMILA_NS {
class Pumila;
/*!
 * \brief fieldに加えてネクスト、落下時間、スコアなども管理する(1プレイヤー分)
 *
 * 時間の単位はフレーム (1/60s)
 *
 */
class GameSim {
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
    /*!
     * \brief rotPairして失敗した場合その回転方向が入る
     * 回転できた場合0
     */
    int rot_fail = 0;
    int rot_fail_count = 0;
    static constexpr int ROT_FAIL_COUNT = 10;

  public:
    bool enable_garbage;

    /*!
     * \brief おじゃまぷよを送る相手をセットしてね
     */
    std::weak_ptr<GameSim> opponent;

    std::shared_ptr<FieldState2> field;
    /*!
     * \brief fieldにアクセスするときはmutexつかってね
     *
     */
    std::shared_mutex field_m;

    /*!
     * \brief 現在の連鎖情報
     *
     * ChainPhaseを抜けるとリセット
     *
     */
    std::optional<Chain> current_chain;

    std::shared_ptr<Pumila> model;
    std::string name;

    int soft_put_interval = 6;
    int soft_put_cnt = 0;

    bool is_over = false;

    PUMILA_DLL explicit GameSim(
        std::shared_ptr<Pumila> model, const std::string &name = "",
        typename std::mt19937::result_type seed = std::random_device()(),
        bool enable_garbage = false);
    explicit GameSim(
        typename std::mt19937::result_type seed = std::random_device()(),
        bool enable_garbage = false)
        : GameSim(nullptr, "", seed, enable_garbage) {}
    GameSim(const GameSim &sim) = delete;
    GameSim(GameSim &&sim) = delete;

    PUMILA_DLL ~GameSim();

    bool hasModel() { return model != nullptr; }

    PUMILA_DLL void reset();

    /*!
     * \brief freePhase時のみぷよを操作する
     *
     * 時間は進まないので適宜step()を呼ぶこと
     *
     */
    PUMILA_DLL void movePair(int dx);
    PUMILA_DLL void rotPair(int r);
    PUMILA_DLL void quickDrop();
    PUMILA_DLL void softDrop();

    /*!
     * \brief 1フレーム時間を進める
     * コンストラクタでmodelをセットしていればそれの返すアクションにしたがってsoftPut()する
     *
     */
    PUMILA_DLL void step();

    /*!
     * \brief ぷよを瞬間移動で置く
     *
     * 時間は進まないので適宜step()を呼ぶこと
     *
     */
    PUMILA_DLL void put(const Action &action);
    /*!
     * \brief ぷよを目標位置まで動かして置くときはここにセットしてstep()を呼ぶ
     */
    PUMILA_DLL void softPut(const Action &action);

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
            garbage,
        };
        virtual PhaseEnum get() const = 0;
    };
    std::unique_ptr<Phase> phase;

    PUMILA_DLL bool isFreePhase();

    struct GarbagePhase final : Phase {
        PUMILA_DLL explicit GarbagePhase(GameSim *sim);
        PhaseEnum get() const override { return PhaseEnum::garbage; }
        PUMILA_DLL std::unique_ptr<Phase> step() override;
        static constexpr int WAIT_T = 20;
        int wait_t;
        inline static std::mt19937 rnd{std::random_device()()};
    };
    struct FreePhase final : Phase {
        PUMILA_DLL explicit FreePhase(GameSim *sim);
        PhaseEnum get() const override { return PhaseEnum::free; }
        PUMILA_DLL std::unique_ptr<Phase> step() override;
        static constexpr int PUT_T = 100;
        static constexpr double FALL_SPEED = 1.0;
        static constexpr double SOFT_SPEED = 25.0;
        PuyoPair current_pair;
        int put_t;
    };
    struct FallPhase final : Phase {
        PUMILA_DLL explicit FallPhase(GameSim *sim);
        PhaseEnum get() const override { return PhaseEnum::fall; }
        PUMILA_DLL std::unique_ptr<Phase> step() override;
        static constexpr int WAIT_T = 20;
        int wait_t;
    };
    struct ChainPhase final : Phase {
        PUMILA_DLL explicit ChainPhase(GameSim *sim);
        PhaseEnum get() const override { return PhaseEnum::chain; }
        PUMILA_DLL std::unique_ptr<Phase> step() override;
        static constexpr int WAIT_T = 30;
        int wait_t;
    };
};

} // namespace PUMILA_NS
