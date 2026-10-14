#include "pc_platform.h"
#include "pc_window_icon.h"

// Vendored, pinned: see pc/lib/stb/README.md
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "../lib/stb/stb_image.h"

void pc_window_icon_set(SDL_Window* window) {
    int w, h, channels;
    // Relative: main() already changed to the exe's folder
    unsigned char* pixels = stbi_load("icon.png", &w, &h, &channels, 4);
    if (pixels == NULL) {
        return; // optional, keep the default icon
    }
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom(pixels, w, h, 32, w * 4, SDL_PIXELFORMAT_RGBA32);
    if (surface != NULL) {
        SDL_SetWindowIcon(window, surface);
        SDL_FreeSurface(surface);
    }
    stbi_image_free(pixels);
}
