#include "chip8.hpp"
#include <iostream>

int main(int argc, char**argv) {
  if(argc < 2) {
    std::cerr<<"Usage --- chip8 <path-to-rom>"<<std::endl;
    return 1;
  }
  Chip8 chip(argv[1]);
  return 0;
}
