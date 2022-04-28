//
//  camera.h
//  worms
//
//  Created by George Watson on 17/06/2021.
//  Copyright © 2021 George Watson. All rights reserved.
//

#ifndef camera_h
#define camera_h
#include "linalgb.h"
#include "graphics.h"

#define TILE_WIDTH 32
#define TILE_HEIGHT 16
#define HALF_TILE_WIDTH  16
#define HALF_TILE_HEIGHT 8

#define CHUNK_WH 512
#define CHUNK_SZ 262144
#define CHUNK_W (CHUNK_WH * TILE_WIDTH)
#define CHUNK_H (CHUNK_WH * HALF_TILE_HEIGHT)

struct camera_t {
  vec2 pos_tl, pos_tr, pos;
  vec2 mpos, grid_mpos, grid_mpos_screen;
  vec2 chunk_pos, chunk_pos_tl, chunk_pos_tr, inner_chunk_pos;
  int chunk_corner;
  vec2 render_sz, render_sz_half, visibility;
  struct surface_t *mask;
};

void camera(struct camera_t *c, int rw, int rh, vec2 pos, struct surface_t *mask);
void camera_set_pos(struct camera_t *c, float x, float y);
void camera_move(struct camera_t *c, float x, float y);
void camera_mpos(struct camera_t *c, vec2 mp);
void camera_update(struct camera_t *c);
vec2 screen_to_map(struct camera_t *c, vec2 p);
vec2 map_to_screen(struct camera_t *c, vec2 p);
vec2 point_to_chunk(vec2 p);

#endif /* camera_h */
