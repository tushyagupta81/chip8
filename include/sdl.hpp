#pragma once

#include "SDL3/SDL_init.h"
#include "structs.hpp"

class SDL_app {
  SDL_t state;

public:
  SDL_AppResult init(uint32_t, uint32_t, uint32_t);
  void clear_screen(uint32_t);
  void update_screen(uint32_t, uint32_t, uint32_t, bool[32][64]);
  ~SDL_app();
};
