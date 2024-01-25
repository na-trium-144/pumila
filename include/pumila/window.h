#pragma once
#include "game.h"
#include <memory>

namespace pumila {
class Window {
    void *sdl_window_p = nullptr, *sdl_renderer_p = nullptr,
         *ttf_font_p = nullptr;
    static constexpr int WIDTH = 500, HEIGHT = 500;
    static constexpr int PUYO_SIZE = 30;
    static constexpr int FIELD_X = 50, FIELD_Y = HEIGHT - 100;
    static int puyoX(double x) { return FIELD_X + PUYO_SIZE * (2 * x + 1) / 2; }
    static int puyoY(double y) { return FIELD_Y - PUYO_SIZE * (2 * y + 1) / 2; }

    std::shared_ptr<GameSim> sim;

    struct KeyState {
        bool soft_drop = false, quick_drop = false, left = false, right = false,
             rot_left = false, rot_right = false;
        std::shared_ptr<GameSim> sim;
        int key_repeat_wait = 0;
        static constexpr int KEY_REPEAT = 10;
        explicit KeyState(const std::shared_ptr<GameSim> &sim) : sim(sim) {}
        void keyFrame();
    } key_state;

    void handleEvent();
    void draw();
    void drawPuyo(Puyo p, double x, double y, bool not_ghost);

  public:
    explicit Window(const std::shared_ptr<GameSim> &sim);
    Window(const Window &win) = delete;
    Window(Window &&win) = delete;

    void loop();
};
} // namespace pumila
