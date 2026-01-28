#pragma once

#include <cstdint>

class Chip8 {
private:
  uint8_t mem[4096];
  bool display[32][64];
  uint16_t pc;
  uint16_t i;
  uint16_t stack[12];
  uint16_t *stp;
  uint8_t delay;
  uint8_t sound;
  uint8_t gpr[16];

public:
  Chip8(char*);

};
