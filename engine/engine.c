//
//  engine.c
//  worms
//
//  Created by George Watson on 30/05/2021.
//  Copyright © 2021 George Watson. All rights reserved.
//

#include "engine.h"

static void app_reset_input() {
  memset(&app()->keyboard.keys, 0, KB_KEY_LAST);
  memset(&app()->prev_keyboard.keys, 0, KB_KEY_LAST);
  memset(&app()->mouse.btns, 0, MOUSE_LAST - 1);
  memset(&app()->prev_mouse.btns, 0, MOUSE_LAST - 1);
}

static void on_error(enum graphics_error type, const char* msg, const char* file, const char* func, int line) {
  fprintf(stderr, "ERROR ENCOUNTERED: %s\nFrom %s, in %s() at %d\n", msg, file, func, line);
  abort();
}

static void on_keyboard(void* _, enum key_sym sym, enum key_mod mod, bool down) {
  app()->prev_keyboard.mod = app()->keyboard.mod;
  app()->keyboard.keys[sym] = down;
  app()->keyboard.mod = mod;
}

static void on_mouse_btn(void* _, enum button btn, enum key_mod mod, bool down) {
  app()->mouse.btns[btn - 1] = down;
  app()->mouse.mod = mod;
}

static void on_mouse_move(void* _, int x, int y, int dx, int dy) {
  app()->prev_mouse.pos = app()->mouse.pos;
  app()->mouse.pos = vec2_new(x, y);
}

static void on_scroll(void* _, enum key_mod mod, float dx, float dy) {
  app()->prev_mouse.wheel = app()->mouse.wheel;
  app()->prev_mouse.mod = app()->mouse.mod;
  app()->mouse.wheel = vec2_new(dx, dy);
  app()->mouse.mod = mod;
}

static void on_focus(void* _, bool focused) {
  app()->window_focused = focused;
}

static void on_resize(void* _, int w, int h) {
  app()->window.w = w;
  app()->window.h = h;
}

static void on_closed(void* _) {
  
}

struct app_t* app(void) {
  static struct app_t *ctx = NULL;
  if (!ctx) {
    ctx = malloc(sizeof(struct app_t));
    graphics_error_callback(on_error);
    window(&ctx->window.ctx, "test", 512, 512, 0);
    window_callbacks(on_keyboard, on_mouse_btn, on_mouse_move, on_scroll,
                     on_focus, on_resize, on_closed, &ctx->window.ctx);
    surface(&ctx->buffer, 512, 512);
    ctx->window.render_w = 512;
    ctx->window.render_h = 512;
    ctx->window_focused  = true;
    ctx->state_stack.top = -1;
    cursor_visible(&app()->window.ctx, false);
  }
  return ctx;
}

struct state_t* app_top_state() {
  return app()->state_stack.states[app()->state_stack.top];
}

void app_push_state(struct state_t *state) {
  app()->state_stack.states[++app()->state_stack.top] = state;
}

struct state_t* app_pop_state() {
  if (app()->state_stack.top == -1)
    return NULL;
  struct state_t *top = app_top_state();
  top->dtor();
  app()->state_stack.top--;
  return top;
}

static int debug_depth = 0;

int app_run(struct state_t *_state) {
  app_reset_input();
  app_push_state(_state);
  unsigned long long last = ticks();
  struct state_t *state;
  while (!closed(&app()->window.ctx) && (state = app_top_state())) {
    events();
    unsigned long long now = ticks();
    double dt = (double)(now - last) / 1000000.0;
    last = now;
    
    switch (state->tick(dt)) {
      case pop:
        free(app_pop_state());
        app_reset_input();
        continue;
#define X(x) \
      case x: \
        app_push_state(new_##x##_state()); \
        app_reset_input(); \
        break;
      STATES
#undef X
      case loop:
      default:
        memcpy(&app()->prev_keyboard.keys, &app()->keyboard.keys, sizeof(bool) * KB_KEY_LAST);
        app()->prev_keyboard.mod = app()->keyboard.mod;
        memcpy(&app()->prev_mouse.btns, &app()->mouse.btns, sizeof(bool) * (MOUSE_LAST - 1));
        app()->prev_mouse.mod = app()->mouse.mod;
        app()->prev_mouse.pos = app()->mouse.pos;
        app()->prev_mouse.wheel = app()->mouse.wheel;
        break;
    }
    
    state->draw();
    debug_depth = 0;
    
    flush(&app()->window.ctx, &app()->buffer);
  }
  
  window_destroy(&app()->window.ctx);
  surface_destroy(&app()->buffer);
  free(app());
  return 1;
}

struct surface_t* app_buffer() {
  return &app()->buffer;
}

void debug_writeln(const char *fmt, ...) {
  char *buf = NULL;
  int buf_sz = 0;
  
  va_list va;
  va_start(va, fmt);
  int len = vsnprintf(buf, buf_sz, fmt, va);
  va_end(va);
  
  if (len + 1 > buf_sz) {
    buf_sz = len + 1;
    buf = realloc(buf, buf_sz);
    va_start(va, fmt);
    vsnprintf(buf, buf_sz, fmt, va);
    va_end(va);
  }
  
  va_list args;
  va_start(args, fmt);
  writeln(&app()->buffer, 0, debug_depth++ * 8, WHITE, BLACK, buf);
  va_end(args);
  if (buf)
    free(buf);
}

bool is_window_focused() {
  return app()->window_focused;
}

bool is_key_down(enum key_sym key) {
  return app()->keyboard.keys[key];
}

bool is_key_up(enum key_sym key) {
  return !!app()->keyboard.keys[key];
}

bool are_keys_down(int n, ...) {
  va_list keys;
  va_start(keys, n);
  bool ret = true;
  for (int i = 0, k = va_arg(keys, int); i < n; ++i, k = va_arg(keys, int))
    if (!app()->keyboard.keys[k]) {
      ret =  false;
      break;
    }
  va_end(keys);
  return ret;
}

bool any_keys_down(int n, ...) {
  va_list keys;
  va_start(keys, n);
  bool ret = false;
  for (int i = 0, k = va_arg(keys, int); i < n; ++i, k = va_arg(keys, int))
    if (app()->keyboard.keys[k]) {
      ret = true;
      break;
    }
  va_end(keys);
  return ret;
}


bool is_button_down(enum button button) {
  return app()->mouse.btns[button];
}

bool is_button_up(enum button button) {
  return !!app()->mouse.btns[button];
}

bool is_key_mod_down(enum key_mod mod) {
  return app()->keyboard.mod & mod;
}

vec2 scroll_wheel(void) {
  return app()->mouse.wheel;
}

vec2 mouse_pos(void) {
  return app()->mouse.pos;
}

vec2 mouse_delta(void) {
  return vec2_sub(app()->mouse.pos, app()->prev_mouse.pos);
}
