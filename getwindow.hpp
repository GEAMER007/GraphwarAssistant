#pragma once
#include "globals.hpp"

const int arena_x = 15;
const int arena_y = 15;
const int arena_w = 770;
const int arena_h = 450;

bool check_graphwar_running( HWND &hwnd );
bool copy_arena_buffer( HWND &hwnd, HDC &our_dc, int x, int y );