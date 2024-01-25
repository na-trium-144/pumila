#pragma once
#ifdef PUMILA_SDL2
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <vector>

namespace pumila::drawing {

struct Color {
    int r, g, b;
};
inline Color col(Puyo p) {
    switch (p) {
    case Puyo::red:
        return Color{255, 120, 120};
    case Puyo::blue:
        return Color{120, 120, 255};
    case Puyo::green:
        return Color{120, 255, 120};
    case Puyo::yellow:
        return Color{255, 255, 0};
    case Puyo::purple:
        return Color{255, 0, 255};
    default:
        return Color{0, 0, 0};
    }
}

using Points = std::vector<SDL_Point>;
inline Points drawCircle(int cx, int cy, int r) {
    Points points;
    for (int y = -r; y <= r; y++) {
        for (int x = -r; x <= r; x++) {
            if (x * x + y * y <= r * r) {
                points.push_back({cx + x, cy + y});
            }
        }
    }
    return points;
}

inline SDL_Texture *drawText(SDL_Renderer *renderer, const std::string &text,
                             TTF_Font *font, SDL_Color textColor, int *width,
                             int *height) {
    SDL_Surface *surface = TTF_RenderText_Solid(font, text.c_str(), textColor);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    *width = surface->w;
    *height = surface->h;
    SDL_FreeSurface(surface);
    return texture;
}
} // namespace pumila::drawing
#endif
