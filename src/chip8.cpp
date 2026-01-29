#include "chip8.hpp"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_scancode.h"
#include "SDL3/SDL_timer.h"
#include "sdl.hpp"
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>

Chip8::Chip8(char *rom_path) {
  this->config = {
      .width = 64,
      .height = 32,
      .fg_color = 0xFFFFFFFF,
      .bg_color = 0x000000FF,
      .scaling_factor = 20,
      .inst_per_sec = 700,
  };

  this->sdl = SDL_app();

  if (this->sdl.init(this->config.width, this->config.height,
                     this->config.scaling_factor) == SDL_APP_FAILURE) {
    std::cerr << "Failed to initialize SDL" << std::endl;
    exit(1);
  }

  uint16_t entry_point = 0x200;
  memset(this->mem, 0, sizeof(mem));

  uint8_t font[80] = {
      0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
      0x20, 0x60, 0x20, 0x20, 0x70, // 1
      0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
      0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
      0x90, 0x90, 0xF0, 0x10, 0x10, // 4
      0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
      0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
      0xF0, 0x10, 0x20, 0x40, 0x40, // 7
      0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
      0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
      0xF0, 0x90, 0xF0, 0x90, 0x90, // A
      0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
      0xF0, 0x80, 0x80, 0x80, 0xF0, // C
      0xE0, 0x90, 0x90, 0x90, 0xE0, // D
      0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
      0xF0, 0x80, 0xF0, 0x80, 0x80  // F
  };
  memcpy(this->mem + 0x050, font, sizeof(font));

  this->pc = entry_point;
  this->stp = &(this->stack[0]);

  std::ifstream rom(rom_path, std::ios::binary | std::ios::ate);
  auto file_size = rom.tellg();
  rom.seekg(0);

  if (file_size <= 0) {
    std::cerr << "Rom of 0KB" << std::endl;
    exit(1);
  }
  const long max_rom_size = sizeof(this->mem) - entry_point;

  if (file_size > max_rom_size) {
    std::cerr << "Rom too big: " << file_size << std::endl;
    exit(1);
  }
  rom.read(reinterpret_cast<char *>(this->mem + entry_point), max_rom_size);

  rom.close();

  this->sound = 0;
  this->delay = 0;

  memset(this->gpr, 0, sizeof(this->gpr));

  this->state = EmuState::RUNNING;
}

void Chip8::cycle() {
  this->opcode.inst = (this->mem[this->pc] << 8) | (this->mem[this->pc + 1]);
  this->pc += 2;

  this->opcode.nnn = this->opcode.inst & 0x0FFF;
  this->opcode.nn = this->opcode.inst & 0x00FF;
  this->opcode.n = this->opcode.inst & 0x000F;
  this->opcode.x = (this->opcode.inst & 0x0F00) >> 8;
  this->opcode.y = (this->opcode.inst & 0x00F0) >> 4;

  switch ((this->opcode.inst >> 12) & 0x0F) {
  case 0x00:
    switch (this->opcode.nn) {
    case 0xE0:
      memset(this->display, false, sizeof(this->display));
      break;
    case 0xEE:
      if (this->stp < this->stack) {
        this->stp--;
        this->pc = *(this->stp);
      } else {
        this->state = EmuState::PAUSED;
      }
      break;
    default:
      // No op
      break;
    }
    break;
  case 0x01:
    this->pc = this->opcode.nnn;
    break;

  case 0x02:
    if (this->stp <
        &this->stack[(sizeof(this->stack) / sizeof(this->stack[0])) - 1]) {
      *this->stp = this->pc;
      this->stp++;
      this->pc = this->opcode.nnn;
    } else {
      this->state = EmuState::PAUSED;
    }
    break;
  case 0x03:
    if (this->gpr[this->opcode.x] == this->opcode.nn) {
      this->pc += 2;
    }
    break;
  case 0x04:
    if (this->gpr[this->opcode.x] != this->opcode.nn) {
      this->pc += 2;
    }
    break;
  case 0x05:
    if (this->gpr[this->opcode.x] == this->gpr[this->opcode.y]) {
      this->pc += 2;
    }
    break;
  case 0x06:
    this->gpr[this->opcode.x] = this->opcode.nn;
    break;
  case 0x07:
    this->gpr[this->opcode.x] += this->opcode.nn;
    break;
  case 0x08:
    break;
  case 0x09:
    if (this->gpr[this->opcode.x] != this->gpr[this->opcode.y]) {
      this->pc += 2;
    }
    break;
  case 0x0A:
    this->i = this->opcode.nnn;
    break;
  case 0x0B:
    break;
  case 0x0C:
    break;
  case 0x0D: {
    uint8_t x = this->gpr[this->opcode.x] & 63;
    uint8_t y = this->gpr[this->opcode.y] & 31;
    uint8_t height = this->opcode.n;
    uint8_t sprite_byte;

    this->gpr[0xF] = 0;

    for (uint8_t row = 0; row < height; row++) {
      if (this->i + row > sizeof(this->mem)) {
        break;
      }
      uint8_t curr_y = y + row;
      if (curr_y >= this->config.height) {
        break;
      }

      sprite_byte = this->mem[this->i + row];

      for (uint8_t col = 0; col < 8; col++) {
        uint8_t curr_x = x + col;
        if (curr_x >= this->config.width) {
          break;
        }

        if ((sprite_byte & (0x80 >> col))) {
          if (this->display[curr_y][curr_x]) {
            this->gpr[0xF] = 1;
          }
          this->display[curr_y][curr_x] ^= 1;
        }
      }
    }
    break;
  }
  case 0x0E:
    break;
  case 0x0F:
    break;
  }
}

void Chip8::run() {
  this->sdl.clear_screen(this->config.bg_color);
  uint32_t last_timer_update_tick = SDL_GetTicks();
  const uint32_t timer_update_interval_ms = 1000 / 60; // 60Hz

  uint32_t cycle_start_tick;
  const double ms_per_instruction = 1000.0 / this->config.inst_per_sec;

  while (this->state == EmuState::RUNNING) {
    cycle_start_tick = SDL_GetTicks();

    this->get_input();

    if (this->state == EmuState::PAUSED) {
      this->sdl.update_screen(this->config.fg_color, this->config.bg_color,
                              this->config.scaling_factor, this->display);
      SDL_Delay(100);                          // Reduce CPU usage when paused
      last_timer_update_tick = SDL_GetTicks(); // Prevent timer catch-up burst
      continue;
    }
    if (this->state == EmuState::QUIT) {
      break;
    }

    // --- Emulation Cycle ---
    // Run one instruction
    if ((size_t)this->pc >= sizeof(this->mem) ||
        (size_t)this->pc + 1 >= sizeof(this->mem)) {
      this->state = PAUSED;
      continue;
    }
    this->cycle();

    // --- Timer Updates (at 60Hz) ---
    uint32_t current_ticks = SDL_GetTicks();
    if (current_ticks - last_timer_update_tick >= timer_update_interval_ms) {
      // update_timers(&chip8);
      // Also update screen at roughly 60Hz, coinciding with timer updates
      this->sdl.update_screen(this->config.fg_color, this->config.bg_color,
                              this->config.scaling_factor, this->display);
      last_timer_update_tick =
          current_ticks; // Or last_timer_update_tick +=
                         // timer_update_interval_ms for more stable 60Hz
    }

    // --- Frame Limiting / IPS control ---
    // Calculate how long this instruction cycle took
    uint32_t instruction_time_ms = SDL_GetTicks() - cycle_start_tick;

    // If the instruction executed faster than desired IPS rate, delay
    if (instruction_time_ms < ms_per_instruction) {
      SDL_Delay((uint32_t)(ms_per_instruction - instruction_time_ms));
    }
    // If overall loop is too fast and screen hasn't updated due to timer
    // interval, we might add a small delay here too, but the screen update is
    // tied to timer now. A more robust game loop might separate instruction
    // execution rate from display rate. For now, tying screen update to timer
    // update (both ~60Hz) is a common approach. The IPS is controlled by
    // delaying after each instruction.
  }
}

void Chip8::get_input() {
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_EVENT_QUIT:
      this->state = EmuState::QUIT;
      return; // Exit input handling immediately on quit

    case SDL_EVENT_KEY_DOWN:
      switch (event.key.scancode) {
      case SDL_SCANCODE_ESCAPE:
        this->state = EmuState::QUIT;
        return;
      case SDL_SCANCODE_SPACE: // Toggle pause/run
        if (this->state == EmuState::RUNNING) {
          this->state = EmuState::PAUSED;
          puts("Emulator Paused.");
        } else if (this->state == EmuState::PAUSED) {
          this->state = EmuState::RUNNING;
          puts("Emulator Resumed.");
        }
        break;

      // CHIP-8 Key to QWERTY Mapping
      case SDL_SCANCODE_1:
        this->keypad[0x1] = true;
        break;
      case SDL_SCANCODE_2:
        this->keypad[0x2] = true;
        break;
      case SDL_SCANCODE_3:
        this->keypad[0x3] = true;
        break;
      case SDL_SCANCODE_4:
        this->keypad[0xC] = true;
        break; // C

      case SDL_SCANCODE_Q:
        this->keypad[0x4] = true;
        break;
      case SDL_SCANCODE_W:
        this->keypad[0x5] = true;
        break;
      case SDL_SCANCODE_E:
        this->keypad[0x6] = true;
        break;
      case SDL_SCANCODE_R:
        this->keypad[0xD] = true;
        break; // D

      case SDL_SCANCODE_A:
        this->keypad[0x7] = true;
        break;
      case SDL_SCANCODE_S:
        this->keypad[0x8] = true;
        break;
      case SDL_SCANCODE_D:
        this->keypad[0x9] = true;
        break;
      case SDL_SCANCODE_F:
        this->keypad[0xE] = true;
        break; // E

      case SDL_SCANCODE_Z:
        this->keypad[0xA] = true;
        break; // A (Mapped to Z)
      case SDL_SCANCODE_X:
        this->keypad[0x0] = true;
        break; // 0 (Mapped to X)
      case SDL_SCANCODE_C:
        this->keypad[0xB] = true;
        break; // B (Mapped to C)
      case SDL_SCANCODE_V:
        this->keypad[0xF] = true;
        break; // F (Mapped to V)
      default:
        break; // Ignore other keys
      }
      break;

    case SDL_EVENT_KEY_UP:
      switch (event.key.scancode) {
      // CHIP-8 Key to QWERTY Mapping
      case SDL_SCANCODE_1:
        this->keypad[0x1] = false;
        break;
      case SDL_SCANCODE_2:
        this->keypad[0x2] = false;
        break;
      case SDL_SCANCODE_3:
        this->keypad[0x3] = false;
        break;
      case SDL_SCANCODE_4:
        this->keypad[0xC] = false;
        break;

      case SDL_SCANCODE_Q:
        this->keypad[0x4] = false;
        break;
      case SDL_SCANCODE_W:
        this->keypad[0x5] = false;
        break;
      case SDL_SCANCODE_E:
        this->keypad[0x6] = false;
        break;
      case SDL_SCANCODE_R:
        this->keypad[0xD] = false;
        break;

      case SDL_SCANCODE_A:
        this->keypad[0x7] = false;
        break;
      case SDL_SCANCODE_S:
        this->keypad[0x8] = false;
        break;
      case SDL_SCANCODE_D:
        this->keypad[0x9] = false;
        break;
      case SDL_SCANCODE_F:
        this->keypad[0xE] = false;
        break;

      case SDL_SCANCODE_Z:
        this->keypad[0xA] = false;
        break; // A
      case SDL_SCANCODE_X:
        this->keypad[0x0] = false;
        break; // 0
      case SDL_SCANCODE_C:
        this->keypad[0xB] = false;
        break; // B
      case SDL_SCANCODE_V:
        this->keypad[0xF] = false;
        break; // F
      default:
        break; // Ignore other keys
      }
      break;
    default:
      break; // Ignore other event types
    }
  }
}

Chip8::~Chip8(){
}
