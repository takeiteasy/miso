//
//  camera.c
//  worms
//
//  Created by George Watson on 17/06/2021.
//  Copyright © 2021 George Watson. All rights reserved.
//

#include "camera.h"

void camera(struct camera_t *c, int rw, int rh, vec2 pos, struct surface_t *mask) {
  c->render_sz = vec2_new(rw, rh);
  c->render_sz_half = vec2_div(c->render_sz, 2.f);
  c->visibility = vec2_new(rw / TILE_WIDTH + 3, rh / HALF_TILE_HEIGHT + 3);
  c->pos = pos;
  c->mpos = vec2_zero();
  c->mask = mask;
  camera_update(c);
}

void camera_set_pos(struct camera_t *c, float x, float y) {
  c->pos.x = x;
  c->pos.y = y;
}

void camera_move(struct camera_t *c, float x, float y) {
  c->pos.x += x;
  c->pos.y += y;
}

void camera_mpos(struct camera_t *c, vec2 mp) {
  c->mpos = mp;
}

void camera_update(struct camera_t *c) {
  c->pos_tl = vec2_sub(c->pos, c->render_sz_half);
  c->pos_tr = vec2_add(c->pos, c->render_sz_half);
  c->grid_mpos = screen_to_map(c, c->mpos);
  c->grid_mpos_screen = map_to_screen(c, c->grid_mpos);
  c->chunk_pos = point_to_chunk(c->pos);
  c->chunk_pos_tl = point_to_chunk(c->pos_tl);
  c->chunk_pos_tr = point_to_chunk(c->pos_tr);
  c->inner_chunk_pos = vec2_new((int)floorf(c->pos.x) % CHUNK_W,
                                (int)floorf(c->pos.y) % CHUNK_H);
  if (c->inner_chunk_pos.x < 0)
    c->inner_chunk_pos.x = CHUNK_W + c->inner_chunk_pos.x;
  if (c->inner_chunk_pos.y < 0)
    c->inner_chunk_pos.y = CHUNK_H + c->inner_chunk_pos.y;
  c->chunk_corner = c->inner_chunk_pos.y < CHUNK_H / 2 ? c->inner_chunk_pos.x < CHUNK_W / 2 ? 0 : 2 : c->inner_chunk_pos.x < CHUNK_W / 2 ? 1 : 3;
}

vec2 screen_to_map(struct camera_t *c, vec2 p) {
  int y  = p.y + HALF_TILE_HEIGHT + c->pos_tl.y;
  int x  = p.x + HALF_TILE_WIDTH + c->pos_tl.x;
  int gx = floor((x + TILE_WIDTH) / TILE_WIDTH) - 1;
  int gy = 2 * (floor((y + TILE_HEIGHT) / TILE_HEIGHT) - 1);
  int ox = x % TILE_WIDTH;
  int oy = y % TILE_HEIGHT;
  switch (pget(c->mask, ox, oy)) {
    case RED:
      gy--;
      break;
    case BLUE:
      gy++;
      break;
    case YELLOW:
      gx--;
      gy--;
      break;
    case LIME:
      gx--;
      gy++;
      break;
  }
  return vec2_new(gx, gy);
}

vec2 map_to_screen(struct camera_t *c, vec2 p) {
  int gx = c->pos_tl.x / TILE_WIDTH;
  int gy = c->pos_tl.y / HALF_TILE_HEIGHT;
  float ox = c->pos_tl.x - (gx * TILE_WIDTH);
  float oy = c->pos_tl.y - (gy * HALF_TILE_HEIGHT);
  int dx = p.x - gx, dy = p.y - gy;
  int px = -ox + (dx * TILE_WIDTH) + ((int)p.y % 2 ? HALF_TILE_WIDTH : 0) - HALF_TILE_WIDTH;
  int py = -oy + (dy * HALF_TILE_HEIGHT) - HALF_TILE_HEIGHT;
  return vec2_new(px, py - 1);
}

vec2 point_to_chunk(vec2 p) {
  return vec2_new(floorf(p.x / CHUNK_W),
                  floorf(p.y / CHUNK_H));
}
