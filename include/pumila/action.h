#pragma once
#include <array>

namespace pumila {
enum class Puyo {
    none = 0,
    red,
    blue,
    green,
    yellow,
    purple,
    garbage,
};

/*!
 * \brief ぷよの置き方
 *
 */
struct Action {
    static constexpr int START_X = 2;
    static constexpr int START_Y = 12;
    int x = START_X;
    enum class Rotation {
        vertical = 0,
        horizontal_right = 1,
        vertical_inverse = 2,
        horizontal_left = 3,
    } rot = Rotation::vertical;
};

constexpr int ACTIONS_NUM = 22;
/*!
 * \brief 可能な置き方の組み合わせ全て
 *
 */
extern const std::array<Action, ACTIONS_NUM> actions;

/*!
 * \brief 降ってくる2個組のぷよ
 *
 * bottomが回転中心
 *
 */
struct PuyoPair : Action {
    using Rotation = Action::Rotation;
    /*!
     * \brief bottomの座標
     *
     */
    double y = START_Y;
    /*!
     * \brief ぷよの色
     *
     */
    Puyo bottom = Puyo::none, top = Puyo::none;
    int bottomX() const { return x; }
    double bottomY() const { return y; }
    /*!
     * \brief x, y, rotからtopの座標を計算
     *
     */
    int topX() const;
    double topY() const;

    PuyoPair() = default;
    PuyoPair(Puyo bottom, Puyo top, const Action &action = Action{})
        : Action(action), bottom(bottom), top(top) {}

    PuyoPair(const PuyoPair &pp, const Action &action)
        : Action(action), y(pp.y), bottom(pp.bottom), top(pp.top) {}

    /*!
     * \brief 右にn回回転する
     *
     */
    void rotate(int right);
};

} // namespace pumila