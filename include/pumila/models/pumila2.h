#pragma once
#include "../model_base.h"
#include <Eigen/Dense>
#include <array>
#include <memory>
#include <utility>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <future>

namespace pumila {
/*!
 * * learnStep時、getActionで選んだ手1通りだけでなく22通りすべてを学習対象にした
 * * 色の順番を24通り並べ替えたデータを同様に学習の入力にした
 * * マルチスレッド化
 */
class Pumila2 : public Pumila {
  protected:
    double gamma;

    int batch_count = 0;
    std::mutex learning_m;
    std::condition_variable learning_cond;
    static constexpr int BATCH_SIZE = 10;

    void load(std::istream &is) override;
    void save(std::ostream &os) override;
    std::string name() const override { return "pumila2"; }

  public:
    struct NNModel {
        double alpha; // sigmoid coef
        double learning_rate;

        struct InNodes {
            double bias;
            double field_colors[FieldState::WIDTH * FieldState::HEIGHT * 4];
            double field_chains[FieldState::WIDTH * FieldState::HEIGHT * 4];
            double score_diff;
        };
        static constexpr int IN_NODES = sizeof(InNodes) / sizeof(double);
        static constexpr int HIDDEN_NODES = 200;

        std::shared_ptr<Eigen::MatrixXd> matrix_ih;
        std::shared_ptr<Eigen::VectorXd> matrix_hq;
        mutable std::mutex matrix_m;

        struct NNResult {
            Eigen::MatrixXd in;
            Eigen::MatrixXd hidden;
            Eigen::VectorXd q;
            std::shared_ptr<Eigen::MatrixXd> matrix_ih;
            std::shared_ptr<Eigen::VectorXd> matrix_hq;
        };

        NNModel(double alpha, double learning_rate);
        NNModel(const NNModel &rhs) { *this = rhs; }
        NNModel &operator=(const NNModel &rhs) {
            alpha = rhs.alpha;
            learning_rate = rhs.learning_rate;
            std::lock_guard lock(matrix_m);
            std::lock_guard lock2(rhs.matrix_m);
            matrix_ih = rhs.matrix_ih;
            matrix_hq = rhs.matrix_hq;
            return *this;
        }

        /*!
         * \brief 順伝播
         * \return NNResult型で, hidden, q の行数は in.rows()
         *
         */
        NNResult forward(const Eigen::MatrixXd &in) const;

        /*!
         * \brief 逆伝播
         * \param result 順伝播の結果
         * \param delta それぞれqの誤差 (行数はq.rows()と同じでなければならない)
         *
         */
        void backward(const NNResult &result, const Eigen::VectorXd &diff);

        /*!
         * \brief 色を並べ替えた特徴量を計算
         * \return in.rows() * 24, IN_NODES の行列
         *
         */
        static Eigen::MatrixXd truncateInNodes(const Eigen::MatrixXd &in);
    };
    /*!
     * getAction時はmainを使う
     * learn時はtargetで計算し、mainに反映し、targetをmainで上書き
     */
    NNModel main, target;
    /*!
     * \brief targetを上書きするときのロック
     */
    std::shared_mutex target_m;

    using NNResult = NNModel::NNResult;

    explicit Pumila2(double alpha, double gamma, double learning_rate);
    std::shared_ptr<Pumila2> copy() {
        auto copied =
            std::make_shared<Pumila2>(main.alpha, gamma, main.learning_rate);
        copied->main = main;
        copied->target = target;
        return copied;
    }

    double mean_diff = 0;

    struct InFeatures {
        std::array<FieldState, ACTIONS_NUM> field_next;
        Eigen::MatrixXd in;
    };
    struct InFeatureSingle {
        FieldState field_next;
        Eigen::VectorXd in;
    };
    /*!
     * \brief 現在のフィールドから次の22通りのフィールドと特徴量を計算
     * (Pumila::poolで実行され完了するまで待機)
     * \return 22 * IN_NODES の行列
     */
    static std::future<InFeatures> getInNodes(const FieldState &field);
    static std::future<InFeatures>
    getInNodes(std::shared_future<InFeatures> feature, int feat_a);
    /*!
     * \brief フィールドにaのアクションをしたあとの特徴量を計算
     */
    static InFeatureSingle getInNodeSingle(const FieldState &field, int a);

    /*!
     * \brief 報酬を計算
     *
     */
    static double calcReward(const FieldState &field);

    int getAction(const FieldState &field) override {
        return getActionRnd(field, 0);
    }
    /*!
     * \brief
     * 評価値最大の手を返すのではなく評価値を確率分布としてランダムな手を返す場合もある
     * \param rnd_p ランダムな手を返す割合 0〜1
     *
     */
    virtual int getActionRnd(const FieldState &field, double rnd_p);
    int getActionRnd(const std::shared_ptr<GameSim> &sim, double rnd_p) {
        return getActionRnd(sim->field, rnd_p);
    }

    /*!
     * \brief フィールドから次の手22通りを計算し学習
     * (Pumila::poolで実行される)
     */
    virtual void learnStep(const FieldState &field);
};
} // namespace pumila
