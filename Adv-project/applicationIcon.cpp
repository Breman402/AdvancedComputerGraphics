#include "applicationIcon.hpp"
#include <SDL.h>
#include <cstdio>

bool setApplicationIcon(SDL_Window* window, const char* bmpPath)
{
    if (window == nullptr)
    {
        std::fprintf(stderr, "setApplicationIcon: window is null.\n");
        return false;
    }
    if (bmpPath == nullptr)
    {
        std::fprintf(stderr, "setApplicationIcon: bmpPath is null.\n");
        return false;
    }

    SDL_Surface* iconSurface = SDL_LoadBMP(bmpPath);
    if (!iconSurface)
    {
        std::fprintf(stderr, "Failed to load icon BMP '%s': %s\n", bmpPath, SDL_GetError());
        return false;
    }

    SDL_SetWindowIcon(window, iconSurface);
    SDL_FreeSurface(iconSurface);
    return true;
}
