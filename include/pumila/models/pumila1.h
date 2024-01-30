#pragma once
#include "../model_base.h"
#include <Eigen/Dense>
#include <array>
#include <memory>
#include <utility>
#include <mutex>

namespace pumila {
class Pumila1 : public Pumila {
    double gamma;
    int back_count = 0;

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

        Eigen::MatrixXd matrix_ih;
        Eigen::VectorXd matrix_hq;
        mutable std::mutex matrix_m;
        Eigen::MatrixXd getMatrixIH() const {
            std::lock_guard lock(matrix_m);
            return matrix_ih;
        }
        Eigen::MatrixXd getMatrixHQ() const {
            std::lock_guard lock(matrix_m);
            return matrix_hq;
        }

        struct NNResult {
            Eigen::MatrixXd in;
            Eigen::MatrixXd hidden;
            Eigen::VectorXd q;
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

    } main, target;
    using NNResult = NNModel::NNResult;

    explicit Pumila1(double alpha, double gamma, double learning_rate);
    std::shared_ptr<Pumila1> copy() {
        auto copied =
            std::make_shared<Pumila1>(main.alpha, gamma, main.learning_rate);
        copied->main = main;
        copied->target = target;
        return copied;
    }

    double mean_diff = 0;

    /*!
     * \brief フィールドから特徴量を計算
     * (Pumila::poolで実行され完了するまで待機)
     * \return 22 * IN_NODES の行列
     */
    static std::pair<std::array<FieldState, ACTIONS_NUM>, Eigen::MatrixXd>
    getInNodes(const FieldState &field);

    /*!
     * \brief 報酬を計算
     *
     */
    static double calcReward(const FieldState &field);

    int getAction(const FieldState &field) override;
    /*!
     * \brief 評価値最大の手を返すのではなく評価値を確率分布としてランダムな手を返す
     *
     */
    int getActionRnd(const FieldState &field);

    /*!
     * \brief フィールドから次の手22通りを計算し学習
     * (Pumila::poolで実行される)
     */
    void learnStep(const FieldState &field);
};
} // namespace pumila
