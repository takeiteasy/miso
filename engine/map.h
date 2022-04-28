//
//  map.h
//  worms
//
//  Created by George Watson on 21/06/2021.
//  Copyright © 2021 George Watson. All rights reserved.
//

#ifndef map_h
#define map_h
#include "chunk.h"
#include "camera.h"
#include "random.h"
#include "threads.h"
#include <sqlite3.h>
#include <stdio.h>

#define MAX_CHUNKS 9
#define MAX_VISIBLE_CHUNKS 4

struct map_t {
  struct chunk_t chunks[MAX_CHUNKS];
  int active_chunk, n_visible_chunks;
  vec2 last_chunk_pos;
  int visible_chunks[MAX_VISIBLE_CHUNKS];
  struct camera_t *camera;
  struct surface_t *tiles;
  struct prng_t seed;
  sqlite3 *db;
};

bool map(struct map_t *m, struct camera_t *c, struct surface_t *b);
void map_update(struct map_t *m, float d);
void map_draw(struct map_t *m, struct surface_t *s);
void map_draw_minimap(struct map_t *m, struct surface_t *s);
void map_destroy(struct map_t *m);

#endif /* map_h */
