#pragma once

#include "sdl.hpp"
#include <cstdint>

enum EmuState {
  RUNNING,
  PAUSED,
  QUIT
};

class Chip8 {
private:
  EmuState state;
  uint8_t mem[4096];
  bool display[32][64];
  uint16_t pc;
  uint16_t i;
  uint16_t stack[12];
  uint16_t *stp;
  uint8_t delay;
  uint8_t sound;
  uint8_t gpr[16];
  bool keypad[16];
  bool is_sound_active;
  config_t config;
  SDL_app sdl;
  Instruction opcode;

public:
  Chip8(char *);
  void run();
  void cycle();
  void update_timers();
  void get_input();
  ~Chip8();
};
