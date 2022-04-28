//
//  game.c
//  worms
//
//  Created by George Watson on 30/05/2021.
//  Copyright © 2021 George Watson. All rights reserved.
//

#include "engine.h"
#include "map.h"
#include "entity.h"

static struct surface_t *buffer;
struct prng_t initial_seed;
struct sprite_t cursor, mask, tiles;
struct map_t world;
struct camera_t view;
struct chunk_t test;

void game_ctor() {
  prng(&initial_seed, 0);
  buffer = app_buffer();
  
  sprite(&cursor, "res/hand.json");
  sprite(&mask, "res/mask.bmp");
  sprite(&tiles, "res/tiles.bmp");
  
  camera(&view, CHUNK_WH, CHUNK_WH, vec2_new(CHUNK_W / 2, CHUNK_H / 2), &mask.buffer);
  map(&world, &view, &tiles.buffer);
}

void game_dtor() {
  sprite_destroy(&cursor);
  sprite_destroy(&mask);
}

enum state_return game_tick(float dt) {
  camera_mpos(&view, mouse_pos());
  if (is_button_down(MOUSE_BTN_0)) {
    vec2 md = mouse_delta();
    camera_move(&view, -md.x * dt / 5.f, -md.y * dt / 5.f);
  }
  if (is_key_down(KB_KEY_SPACE))
    camera_set_pos(&view, 0, -1);
  camera_update(&view);
  map_update(&world, dt);
  return loop;
}

void game_draw() {
  fill(buffer, BLACK);
  map_draw(&world, buffer);
  
  debug_writeln("POS:   (x: %0.f, y: %0.f)", view.pos.x, view.pos.y);
  debug_writeln("CHUNK: (x: %0.f, y: %0.f)", view.chunk_pos.x, view.chunk_pos.y);
  debug_writeln("ICP:   (x: %0.f, y: %0.f)", view.inner_chunk_pos.x, view.inner_chunk_pos.y);
  for (int x = view.chunk_pos_tl.x; x < view.chunk_pos_tr.x + 1; ++x)
    for (int y = view.chunk_pos_tl.y; y < view.chunk_pos_tr.y + 1; ++y)
      debug_writeln("VISIBLE CHUNK: (%d, %d)", x, y);
  
  circle(buffer, 256, 256, 2, RED, true);
  circle(buffer, view.mpos.x, view.mpos.y, 2, BLUE, true);
}
