#pragma once

#include "structs.hpp"
#include <SDL3/SDL.h>
#include <atomic>

#define AMPLITUDE 0.25f
#define BEEP_FREQUENCY 350.0f
#define SAMPLE_RATE 8000

class SDL_app {
private:
  SDL_t state{};

  SDL_AudioStream *audio_stream = nullptr;
  std::atomic<bool> s_should_beep_play{false};
  int wave_sample = 0;

  void generate_beep();

public:
  SDL_app() = default;

  SDL_AppResult init(uint32_t, uint32_t, uint32_t);
  bool platform_init_audio();
  void platform_start_beep();
  void platform_stop_beep();

  void clear_screen(uint32_t);
  void update_screen(uint32_t, uint32_t, uint32_t, bool[32][64]);

  ~SDL_app();
};
