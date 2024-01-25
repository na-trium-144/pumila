#include <pumila/window.h>
#include "drawing.h"
#include <iostream>

#ifdef PUMILA_SDL2
#include <SDL2/SDL.h>
#endif

namespace pumila {
Window::Window(const std::shared_ptr<GameSim> &sim) : sim(sim) {
#ifdef PUMILA_SDL2
    SDL_Window *sdl_window = nullptr;
    SDL_Renderer *sdl_renderer = nullptr;
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Couldn't initialize SDL: " << SDL_GetError() << std::endl;
    } else {
        sdl_window =
            SDL_CreateWindow("Pumila", SDL_WINDOWPOS_UNDEFINED,
                             SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, 0);
        sdl_window_p = static_cast<void *>(sdl_window);
        if (!sdl_window) {
            std::cerr << "Couldn't create window: " << SDL_GetError()
                      << std::endl;
        } else {
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
            sdl_renderer =
                SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_ACCELERATED);
            sdl_renderer_p = static_cast<void *>(sdl_renderer);
            if (!sdl_renderer) {
                std::cerr << "Couldn't create renderer: " << SDL_GetError()
                          << std::endl;
            }
        }
    }
    TTF_Init();
    TTF_Font *ttf_font =
        TTF_OpenFont("/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf", 24);
    ttf_font_p = static_cast<void *>(ttf_font);
    if (!ttf_font) {
        std::cerr << "Font not found" << std::endl;
    }
#else
    std::cerr << "Pumila not built with SDL2" << std::endl;
#endif
}

void Window::loop() {
#ifdef PUMILA_SDL2
    if (sdl_window_p && sdl_renderer_p) {
        SDL_Window *sdl_window = static_cast<SDL_Window *>(sdl_window_p);
        SDL_Renderer *sdl_renderer =
            static_cast<SDL_Renderer *>(sdl_renderer_p);
        TTF_Font *ttf_font = static_cast<TTF_Font *>(ttf_font_p);

        bool soft_drop = false;
        while (true) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                switch (event.type) {
                case SDL_QUIT:
                    SDL_Quit();
                    return;
                case SDL_KEYDOWN:
                    if (!event.key.repeat) {
                        switch (event.key.keysym.scancode) {
                        case SDL_SCANCODE_UP:
                        case SDL_SCANCODE_W:
                            sim->quickDrop();
                            break;
                        case SDL_SCANCODE_DOWN:
                        case SDL_SCANCODE_S:
                            soft_drop = true;
                            break;
                        case SDL_SCANCODE_LEFT:
                        case SDL_SCANCODE_A:
                            sim->movePair(-1);
                            break;
                        case SDL_SCANCODE_RIGHT:
                        case SDL_SCANCODE_D:
                            sim->movePair(1);
                            break;
                        case SDL_SCANCODE_KP_DIVIDE:
                        case SDL_SCANCODE_N:
                            sim->rotPair(-1);
                            break;
                        case SDL_SCANCODE_KP_MULTIPLY:
                        case SDL_SCANCODE_M:
                            sim->rotPair(1);
                            break;
                        default:
                            break;
                        }
                    }
                    break;
                case SDL_KEYUP:
                    if (!event.key.repeat) {
                        switch (event.key.keysym.scancode) {
                        case SDL_SCANCODE_DOWN:
                        case SDL_SCANCODE_S:
                            soft_drop = false;
                            break;
                        default:
                            break;
                        }
                    }
                    break;
                default:
                    break;
                }
            }

            sim->step(16, soft_drop);

            using namespace pumila::drawing;
            SDL_SetRenderDrawColor(sdl_renderer, 255, 255, 255, 255);
            SDL_RenderClear(sdl_renderer);

            SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, 255);
            SDL_Rect field_rect = {FIELD_X, FIELD_Y - PUYO_SIZE * 12,
                                   PUYO_SIZE * 6, PUYO_SIZE * 12};
            SDL_RenderDrawRect(sdl_renderer, &field_rect);

            if (sim->phase == GameSim::Phase::free) {
                drawPuyo(sim->current_pair.bottom, sim->current_pair_x,
                         sim->field.putTargetY(sim->current_pair_x), true);
                drawPuyo(sim->current_pair.bottom, sim->current_pair_x,
                         sim->current_pair_y);
                switch (static_cast<Rotation>(sim->current_pair_rot)) {
                case Rotation::vertical:
                    drawPuyo(sim->current_pair.top, sim->current_pair_x,
                             sim->field.putTargetY(sim->current_pair_x) + 1,
                             true);
                    drawPuyo(sim->current_pair.top, sim->current_pair_x,
                             sim->current_pair_y + 1);
                    break;
                case Rotation::vertical_inverse:
                    drawPuyo(sim->current_pair.top, sim->current_pair_x,
                             sim->field.putTargetY(sim->current_pair_x) - 1,
                             true);
                    drawPuyo(sim->current_pair.top, sim->current_pair_x,
                             sim->current_pair_y - 1);
                    break;
                case Rotation::horizontal_left:
                    drawPuyo(sim->current_pair.top, sim->current_pair_x - 1,
                             sim->field.putTargetY(sim->current_pair_x - 1),
                             true);
                    drawPuyo(sim->current_pair.top, sim->current_pair_x - 1,
                             sim->current_pair_y);
                    break;
                case Rotation::horizontal_right:
                    drawPuyo(sim->current_pair.top, sim->current_pair_x + 1,
                             sim->field.putTargetY(sim->current_pair_x + 1),
                             true);
                    drawPuyo(sim->current_pair.top, sim->current_pair_x + 1,
                             sim->current_pair_y);
                    break;
                }
            }

            for (int y = 0; y < FieldState::HEIGHT; y++) {
                for (int x = 0; x < FieldState::WIDTH; x++) {
                    drawPuyo(sim->field.get(x, y), x, y);
                }
            }

            if (ttf_font) {
                int text_w, text_h;
                SDL_Texture *score_t =
                    drawText(sdl_renderer, std::to_string(sim->score).c_str(),
                             ttf_font, {0, 0, 0, 255}, &text_w, &text_h);
                SDL_Rect rect = {FIELD_X + PUYO_SIZE * 6 - text_w - 10,
                                 FIELD_Y + 10, text_w, text_h};
                SDL_RenderCopy(sdl_renderer, score_t, NULL, &rect);
                SDL_DestroyTexture(score_t);

                if (sim->current_chain) {
                    score_t = drawText(
                        sdl_renderer,
                        (std::to_string(sim->last_chain.scoreA()) + " x " +
                         std::to_string(
                             sim->last_chain.scoreB(sim->current_chain)))
                            .c_str(),
                        ttf_font, {0, 0, 0, 255}, &text_w, &text_h);
                    rect = {FIELD_X + PUYO_SIZE * 6 - text_w - 10, FIELD_Y + 40,
                            text_w, text_h};
                    SDL_RenderCopy(sdl_renderer, score_t, NULL, &rect);
                    SDL_DestroyTexture(score_t);

                    score_t = drawText(
                        sdl_renderer,
                        ("(" + std::to_string(sim->current_chain) + ")").c_str(),
                        ttf_font, {0, 0, 0, 255}, &text_w, &text_h);
                    rect = {FIELD_X + 10, FIELD_Y + 40, text_w, text_h};
                    SDL_RenderCopy(sdl_renderer, score_t, NULL, &rect);
                    SDL_DestroyTexture(score_t);
                }
            }

            SDL_RenderPresent(sdl_renderer);
            SDL_Delay(16);
        }
    }
#endif
}
void Window::drawPuyo(Puyo p, double x, double y, bool light) {
#ifdef PUMILA_SDL2
    if (sdl_window_p && sdl_renderer_p && p != Puyo::none) {
        using namespace pumila::drawing;
        SDL_Renderer *sdl_renderer =
            static_cast<SDL_Renderer *>(sdl_renderer_p);
        Points points;
        if (light) {
            SDL_SetRenderDrawColor(sdl_renderer, lightCol(p).r, lightCol(p).g,
                                   lightCol(p).b, 255);
            points = drawCircle(puyoX(x), puyoY(y), PUYO_SIZE * 0.5 / 2);
        } else {
            SDL_SetRenderDrawColor(sdl_renderer, col(p).r, col(p).g, col(p).b,
                                   255);
            points = drawCircle(puyoX(x), puyoY(y), PUYO_SIZE * 0.9 / 2);
        }
        SDL_RenderDrawPoints(sdl_renderer, points.data(), points.size());
    }
#endif
}

} // namespace pumila
