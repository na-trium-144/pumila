#pragma once
#include <utility>
#include <array>
#include <cstdint>
#include <vector>

namespace pumila {
struct Chain;

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
 * \brief 降ってくる2個組のぷよ
 *
 * bottomが回転中心
 *
 */
struct PuyoPair {
    Puyo bottom, top;
    /*!
     * \brief bottomの座標
     *
     */
    int x;
    double y;
    enum class Rotation {
        vertical = 0,
        horizontal_right = 1,
        vertical_inverse = 2,
        horizontal_left = 3,
    } rot;
    int bottomX() const { return x; }
    double bottomY() const { return y; }
    /*!
     * \brief x, y, rotからtopの座標を計算
     *
     */
    int topX() const;
    double topY() const;
    PuyoPair() = default;
    PuyoPair(Puyo bottom, Puyo top)
        : bottom(bottom), top(top), x(2), y(12), rot(Rotation::vertical) {}

    /*!
     * \brief 右にn回回転する
     *
     */
    void rotate(int right);
};

/*!
 * \brief 1プレイヤーの盤面の情報
 *
 * nextやスコアなどは含まない
 *
 */
class FieldState {
  public:
    static constexpr std::size_t WIDTH = 6, HEIGHT = 13;

    static bool inRange(std::size_t x, std::size_t y = 0) {
        return x < WIDTH && y < HEIGHT;
    }

  private:
    std::array<std::array<Puyo, WIDTH>, HEIGHT> field = {};

    void put(std::size_t x, std::size_t y, Puyo p) {
        if (y < HEIGHT && x < WIDTH) {
            field.at(y).at(x) = p;
        }
    }

    /*!
     * \brief x, y とつながっているぷよの数を数え、
     * すでに数えたものをフィールドから消す
     * \return 削除したぷよ
     */
    std::vector<std::pair<std::size_t, std::size_t>>
    deleteConnection(std::size_t x, std::size_t y);

    void
    deleteConnection(std::size_t x, std::size_t y,
                     std::vector<std::pair<std::size_t, std::size_t>> &deleted);

  public:
    FieldState copy() const { return *this; }

    Puyo get(std::size_t x, std::size_t y) const { return field.at(y).at(x); }

    /*!
     * \brief puyopairを落とす
     *
     */
    void put(const PuyoPair &pp);

    /*!
     * \brief puyopairを落とした場合のy座標を調べる
     * \param fall false->ちぎり前、true->ちぎり後の座標
     * \return bottom, topのそれぞれのy座標
     *
     */
    std::pair<std::size_t, std::size_t> putTargetY(const PuyoPair &pp,
                                                   bool fall) const;
    /*!
     * \brief ぷよを1つ落とした場合のy座標を調べる
     *
     */
    std::size_t putTargetY(std::size_t x) const;

    /*!
     * \brief 4連結を探し、消す
     *
     * 盤面に4連結が無かった場合何もせず消したぷよの数は0として返る
     *
     * \return 消したぷよの情報
     *
     */
    Chain deleteChain(int chain_num);

    /*!
     * \brief 盤面の各マスについて消したら何連鎖が起きるかを計算する
     * 
     */
    std::array<std::array<int, WIDTH>, HEIGHT> calcChainAll() const;

    /*!
     * \brief 空中に浮いているぷよを落とす
     *
     * \return 落ちたぷよがあったらtrue
     *
     */
    bool fall();
};
} // namespace pumila
