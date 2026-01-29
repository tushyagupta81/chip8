#include "sdl.hpp"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_log.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_video.h"
#include "structs.hpp"
#include <cstdint>

SDL_AppResult SDL_app::init(uint32_t width, uint32_t height,
                            uint32_t scaling_factor) {
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
    SDL_Log("SDL initialization failed. %s\n", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  this->state.window =
      SDL_CreateWindow("Chip8 Emulator", width * scaling_factor,
                       height * scaling_factor, SDL_WINDOW_RESIZABLE);
  if (!this->state.window) {
    SDL_Log("SDL window creation failed: %s\n", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  this->state.renderer =
      SDL_CreateRenderer(this->state.window, nullptr);
  if (!this->state.renderer) {
    SDL_Log("Renderer creation failed: %s\n", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  return SDL_APP_CONTINUE;
}

SDL_app::~SDL_app() {
  SDL_DestroyWindow(this->state.window);
  SDL_DestroyRenderer(this->state.renderer);
  SDL_Quit();
}

void SDL_app::clear_screen(uint32_t bg_color) {
  uint8_t r = (bg_color >> 24) & 0xFF;
  uint8_t g = (bg_color >> 16) & 0xFF;
  uint8_t b = (bg_color >> 8) & 0xFF;
  uint8_t a = bg_color & 0xFF;

  SDL_SetRenderDrawColor(this->state.renderer, r, g, b, a);
  SDL_RenderClear(this->state.renderer);
}

void SDL_app::update_screen(uint32_t fg_color, uint32_t bg_color,
                            uint32_t scaling_factor, bool display[32][64]) {
  SDL_FRect pixel_rect = {
      .x = 0,
      .y = 0,
      .w = static_cast<float>(scaling_factor),
      .h = static_cast<float>(scaling_factor),
  };

  // Foreground color components
  const uint8_t fg_r = (fg_color >> 24) & 0xFF;
  const uint8_t fg_g = (fg_color >> 16) & 0xFF;
  const uint8_t fg_b = (fg_color >> 8) & 0xFF;
  const uint8_t fg_a = (fg_color >> 0) & 0xFF;

  // Background color components (used for outline if enabled, or drawing "off"
  // pixels)
  const uint8_t bg_r = (bg_color >> 24) & 0xFF;
  const uint8_t bg_g = (bg_color >> 16) & 0xFF;
  const uint8_t bg_b = (bg_color >> 8) & 0xFF;
  const uint8_t bg_a = (bg_color >> 0) & 0xFF;

  for (uint32_t row = 0; row < 32; row++) {
    pixel_rect.y = row * scaling_factor;
    for (uint32_t col = 0; col < 64; col++) {
      pixel_rect.x = col * scaling_factor;
      if (display[row][col]) { // If the CHIP-8 pixel is "on"
        SDL_SetRenderDrawColor(this->state.renderer, fg_r, fg_g, fg_b, fg_a);
        SDL_RenderFillRect(this->state.renderer, &pixel_rect);

        // if (outline) { // Optionally draw an outline for "on" pixels
        //   SDL_SetRenderDrawColor(this->state.renderer, bg_r, bg_g, bg_b,
        //       bg_a); // Outline with background color
        //   SDL_RenderDrawRect(this->state.renderer, &pixel_rect);
        // }
      } else { // If the CHIP-8 pixel is "off"
        SDL_SetRenderDrawColor(this->state.renderer, bg_r, bg_g, bg_b, bg_a);
        SDL_RenderFillRect(this->state.renderer, &pixel_rect);
      }
    }
  }
  SDL_RenderPresent(this->state.renderer); // Show the drawn frame
}
