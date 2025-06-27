#pragma once
#include "globals.hpp"

// gui from my old project, adapted for this one

static auto last_render = 0ull;
static bool full_exit = false;
static long long delta_time = 0;

extern HBRUSH   brush_background;
extern HBRUSH   brush_red;
extern HBRUSH   brush_transparent;
extern HPEN     pen_background;
extern HPEN     pen_green;
extern HDC      hdc_mem;
extern HBITMAP  hbm_mem;
extern HWND     g_hwnd;

static const int initial_width = 790;
static const int initial_height = 600;

static int      start_width = initial_width;
static int      start_height = initial_height;
static int      target_width = initial_width;
static int      target_height = initial_height;
static bool     is_resizing = false;
static float    anim_progress = 0.0f;
static int      curr_width = initial_width;
static int      curr_height = initial_height;

static const float timer_interval = 5.0f;

static const COLORREF COLOR_BG = 0x1a'1a'1a;
static const COLORREF COLOR_WHITE = RGB( 255, 255, 255 );

static constexpr double PI = 3.14159265358979323846;
static constexpr double DEG2RAD = PI / 180.0;



LRESULT CALLBACK wnd_proc( HWND, UINT, WPARAM, LPARAM );
void init_double_buffer( HWND );
void cleanup_double_buffer( );
void init_resources( );
void cleanup_resources( );
unsigned long __stdcall init_gui( LPVOID lparam );
void draw_frame( HDC &hdc );