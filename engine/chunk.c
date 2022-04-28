//
//  chunk.c
//  worms
//
//  Created by George Watson on 29/05/2021.
//  Copyright © 2021 George Watson. All rights reserved.
//

#include "chunk.h"

#define idx(x, y) ((y) * CHUNK_WH + (x))

void chunk(struct chunk_t *c, int x, int y, int seed) {
  prng(&c->rnd, seed);
  c->x = x;
  c->y = y;
  
  float z_seed = rnd_float(&c->rnd);
  float scale = 200.f;
  float lacunarity = 2.f;
  float gain = .5f;
  int octaves = 16;
  bool circle = false;
  
  for (int x = 0; x < CHUNK_WH; ++x)
    for (int y = 0; y < CHUNK_WH; ++y) {
      float freq = 2.f,
            amp  = 1.0f,
            tot  = 0.0f,
            sum  = 0.0f;
      for (int i = 0; i < octaves; ++i) {
        sum  += simplex((x / scale) * freq, (y / scale) * freq, z_seed) * amp;
        tot  += amp;
        freq *= lacunarity;
        amp  *= gain;
      }
      
      float d  = CHUNK_WH * .5f;
      float dx = fabs(x - d);
      float dy = fabs(y - d);
      float grad = powf((circle ? sqrt(dx * dx + dy * dy) : max(dx, dy)) / (d - 10.0f), 2);
      sum = clamp((sum / tot), 0., 1.f) * max(0.0f, 1.0f - grad);
      
      if (sum < 0.1)
        c->tiles[idx(x, y)] = WATER;
      else if (sum < 0.12)
        c->tiles[idx(x, y)] = WATER; // Shallow water?
      else if (sum < 0.15)
        c->tiles[idx(x, y)] = SAND;
      else if (sum < 0.30)
        c->tiles[idx(x, y)] = GRASS;
      else if (sum < 0.44)
        c->tiles[idx(x, y)] = JUNGLE;
      else
        c->tiles[idx(x, y)] = WATER;
    }
}

void chunk_draw(struct chunk_t *c, vec2 pos, vec2 visible, struct surface_t *a, struct surface_t *s) {
  int gx = pos.x / TILE_WIDTH, gy = pos.y / HALF_TILE_HEIGHT;
  float ox = pos.x - (gx * TILE_WIDTH), oy = pos.y - (gy * HALF_TILE_HEIGHT);
  
  for (int y = 0; y < visible.y; ++y)
    for (int x = 0; x < visible.x; ++x) {
      int px = gx + x, py = gy + y;
      if (px < 0 || py < 0 || px >= CHUNK_WH || py >= CHUNK_WH)
        continue;
      clip_paste(s, a, -ox + (x * TILE_WIDTH) + (py % 2 ? HALF_TILE_WIDTH : 0) - HALF_TILE_WIDTH, -oy + (y * HALF_TILE_HEIGHT) - HALF_TILE_HEIGHT, 0, (int)c->tiles[idx(px, py)] * TILE_HEIGHT, TILE_WIDTH, TILE_HEIGHT);
    }
}

void chunk_destroy(struct chunk_t *c) {
  memset(c->tiles, 0, CHUNK_SZ);
  memset(c, 0, sizeof(struct chunk_t));
}
