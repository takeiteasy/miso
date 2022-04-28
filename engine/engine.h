//
//  engine.h
//  worms
//
//  Created by George Watson on 30/05/2021.
//  Copyright © 2021 George Watson. All rights reserved.
//

#ifndef engine_h
#define engine_h
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "graphics.h"
#include "linalgb.h"
#include "sprite.h"

#define STATES \
  X(menu) \
  X(game)

struct mouse_hander_t {
  bool btns[MOUSE_LAST - 1];
  vec2 pos, wheel;
  enum key_mod mod;
};

struct keyboard_handler_t {
  bool keys[KB_KEY_LAST];
  enum key_mod mod;
};

struct window_handler_t {
  struct window_t ctx;
  int w, h, render_w, render_h;
};

enum state_return {
  loop,
  pop,
#define X(x) x,
  STATES
#undef X
};

struct state_t {
  void(*ctor)(void);
  void(*dtor)(void);
  enum state_return(*tick)(float);
  void(*draw)(void);
};

#define X(x) \
void x##_ctor(void); \
void x##_dtor(void); \
enum state_return x##_tick(float dt); \
void x##_draw(void); \
static struct state_t* new_##x##_state(void) { \
  struct state_t *ret = malloc(sizeof(struct state_t)); \
  ret->ctor = x##_ctor; \
  ret->dtor = x##_dtor; \
  ret->tick = x##_tick; \
  ret->draw = x##_draw; \
  ret->ctor(); \
  return ret; \
}
STATES
#undef X

#define MAX_STATE_STACK_SIZE 32

struct state_stack_t {
  struct state_t *states[MAX_STATE_STACK_SIZE];
  int top;
};

struct app_t {
  struct window_handler_t window;
  struct surface_t buffer;
  struct mouse_hander_t mouse, prev_mouse;
  struct keyboard_handler_t keyboard, prev_keyboard;
  struct state_stack_t state_stack;
  bool window_focused;
};

struct app_t* app(void);
struct state_t* app_top_state(void);
void app_push_state(struct state_t *state);
struct state_t* app_pop_state(void);
int app_run(struct state_t *state);
#define RUN(x) app_run(new_##x##_state())

struct surface_t* app_buffer(void);
void debug_writeln(const char *fmt, ...);
bool is_window_focused(void);
bool is_key_down(enum key_sym key);
bool is_key_up(enum key_sym key);
bool are_keys_down(int n, ...);
bool any_keys_down(int n, ...);
double ticks_since_key_down(enum key_sym key);
double ticks_since_key_up(enum key_sym key);
bool is_button_down(enum button button);
bool is_button_up(enum button button);
double ticks_since_button_down(enum button button);
double ticks_since_button_up(enum button button);
bool is_key_mod_down(enum key_mod mod);
vec2 scroll_wheel(void);
vec2 mouse_pos(void);
vec2 mouse_delta(void);

#endif /* engine_h */
