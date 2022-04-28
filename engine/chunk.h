//
//  chunk.h
//  worms
//
//  Created by George Watson on 29/05/2021.
//  Copyright © 2021 George Watson. All rights reserved.
//

#ifndef chunk_h
#define chunk_h
#include "graphics.h"
#include "linalgb.h"
#include "sprite.h"
#include "camera.h"
#include "random.h"
#include <stdlib.h>
#include <string.h>

enum tile {
  WATER  = 0,
  GRASS  = 1,
  SAND   = 2,
  JUNGLE = 3
};

struct chunk_t {
  enum tile tiles[CHUNK_SZ];
  struct prng_t rnd;
  int x, y;
};

void chunk(struct chunk_t *c, int x, int y, int seed);
void chunk_draw(struct chunk_t *c, vec2 pos, vec2 visible, struct surface_t *a, struct surface_t *s);
void chunk_destroy(struct chunk_t *c);

#endif /* chunk_h */
