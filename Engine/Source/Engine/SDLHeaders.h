#pragma once

// We disable the preprocessor define to rename main to SDL_main, and do it ourselves
#define SDL_MAIN_HANDLED 1
#include <SDL.h>

#if Y_COMPILER_MSVC
    #pragma comment(lib, "SDL2.lib")
#endif

