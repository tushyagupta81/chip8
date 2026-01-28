cmake_minimum_required(VERSION 4.2)

project(Chip8)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

add_subdirectory(libs/SDL EXCLUDE_FROM_ALL)

add_executable(Chip8)

target_link_libraries(Chip8 PRIVATE SDL3::SDL3)

target_sources(Chip8
  PRIVATE
   src/main.cpp
   src/chip8.cpp
)

 target_include_directories(Chip8
   PRIVATE
   include)
