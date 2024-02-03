#pragma once
#include "def.h"
#include "game.h"
#include <memory>

namespace pumila {
class Window {
    void *sdl_window_p = nullptr, *sdl_renderer_p = nullptr,
         *ttf_font_p = nullptr, *ttf_font_sm_p = nullptr;
    static constexpr int WIDTH = 700;
    static constexpr int HEIGHT = 500;
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
             rot_left = false, rot_right = false;
        std::vector<std::shared_ptr<GameSim>> sim;
        int key_repeat_wait = 0;
        static constexpr int KEY_REPEAT = 10;
        explicit KeyState(const std::vector<std::shared_ptr<GameSim>> &sim)
            : sim(sim) {}
        PUMILA_DLL void keyFrame();
    } key_state;

    PUMILA_DLL void handleEvent();
    PUMILA_DLL void draw();
    PUMILA_DLL void drawPuyo(Puyo p, double x, double y, int i, bool not_ghost);

  public:
    explicit Window(const std::shared_ptr<GameSim> &sim)
        : Window(std::vector<std::shared_ptr<GameSim>>{sim}) {}
    PUMILA_DLL explicit Window(const std::vector<std::shared_ptr<GameSim>> &sim);
    PUMILA_DLL ~Window();
    Window(const Window &win) = delete;
    Window(Window &&win) = delete;

    /*!
     * \brief 画面更新をする
     *
     * \param sim_step 画面更新周期にあわせてsim.step()を回すかどうか
     *
     */
    PUMILA_DLL void step(bool sim_step);
    PUMILA_DLL void quit();
    PUMILA_DLL bool isRunning() const;
};
} // namespace pumila
