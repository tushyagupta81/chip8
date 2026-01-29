#pragma once
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_video.h"
#include <cstdint>

typedef struct {
  SDL_Window *window;
  SDL_Renderer *renderer;
} SDL_t;

typedef struct {
  uint32_t width;
  uint32_t height;
  uint32_t fg_color;
  uint32_t bg_color;
  uint32_t scaling_factor;
  int inst_per_sec;
} config_t;

typedef struct {
  uint16_t inst;
  uint8_t x;
  uint8_t y;
  uint8_t n;
  uint8_t nn;
  uint16_t nnn;
} Instruction;
