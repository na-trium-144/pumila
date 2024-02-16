#pragma once
#include "def.h"
#include "game.h"
#include <memory>

namespace PUMILA_NS {
class Window {
    void *sdl_window_p = nullptr, *sdl_renderer_p = nullptr,
         *ttf_font_p = nullptr, *ttf_font_sm_p = nullptr;
    static constexpr int WIDTH = 700;
    static constexpr int HEIGHT = 560;
    static constexpr int PUYO_SIZE = 30;
    static constexpr std::array<int, 2> FIELD_X = {50, 400};
    static constexpr int FIELD_Y = HEIGHT - 100;
    static int puyoX(double x, int i) {
        return FIELD_X[i] + PUYO_SIZE * (2 * x + 1) / 2;
    }
    static int puyoY(double y) { return FIELD_Y - PUYO_SIZE * (2 * y + 1) / 2; }

    std::vector<std::shared_ptr<GameSim>> sim;

    struct KeyState {
        bool soft_drop = false, quick_drop = false, left = false, right = false,
             rot_left = false, rot_right = false, reset = false;
        Window *win;
        int key_repeat_wait = 0;
        static constexpr int KEY_REPEAT = 10;
        explicit KeyState(Window *win) : win(win) {}
        PUMILA_DLL void keyFrame();
    } key_state;

    PUMILA_DLL void handleEvent();
    PUMILA_DLL void draw();
    PUMILA_DLL void drawPuyo(Puyo p, double x, double y, int i, bool not_ghost);

  public:
    explicit Window(const std::shared_ptr<GameSim> &sim)
        : Window(std::vector<std::shared_ptr<GameSim>>{sim}) {}
    PUMILA_DLL explicit Window(
        const std::vector<std::shared_ptr<GameSim>> &sim);
    PUMILA_DLL ~Window();
    Window(const Window &win) = delete;
    Window(Window &&win) = delete;

    enum class WindowState {
        ready, //!< 開始前、READY_T経過でgameに移行
        game,  //!< ゲーム中、simがゲームオーバーでfinishに移行
        finish, //!< 終了
    } state;

    static constexpr int READY_T = 60;
    static constexpr int GO_T = 60;
    int phase_t = 0;

    /*!
     * \brief 画面更新をする & 1フレームsleep
     * \param sim_step 画面更新周期にあわせてsim.step()を回すかどうか
     * \param state 画面に表示する文字
     */
    PUMILA_DLL void step(bool sim_step);
    PUMILA_DLL void quit();
    PUMILA_DLL bool isRunning() const;
};
} // namespace PUMILA_NS
