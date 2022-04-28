//
//  map.c
//  worms
//
//  Created by George Watson on 21/06/2021.
//  Copyright © 2021 George Watson. All rights reserved.
//

#include "map.h"

static const char *create_db = "CREATE TABLE chunks(id x, id y, data Blob);";

static bool exec_sql(sqlite3 *db, const char *cmd) {
  char *err = NULL;
  if (!sqlite3_exec(db, cmd, NULL, NULL, &err)) {
    printf("SQL ERROR: %s\n", err);
    sqlite3_free(err);
    return false;
  }
  return true;
}

static bool vec2_cmp(vec2 a, vec2 b) {
  return a.x == b.x && a.y == b.y;
}

static int bearing(float x1, float y1, float x2, float y2) {
  float l1 = radians(x1);
  float l2 = radians(x2);
  float dl = radians(y2 - y1);
  return (int)roundf((((int)degrees(atan2f(sinf(dl) * cosf(l2), cosf(l1) * sinf(l2) - (sinf(l1) * cosf(l2) * cosf(dl)))) + 360) % 360) / 45.f);
}

static void map_load_chunk(struct map_t *m, int x, int y) {
  
}

static int map_find_chunk(struct map_t *m, int x, int y) {
  return 0;
}

static void map_deload_chunk(struct map_t *m, int x, int y) {
  int n = map_find_chunk(m, x, y);
}

struct chunk_thread_t {
  struct chunk_t *chunk;
  int x, y;
  struct prng_t *rnd;
};

static int chunk_thread(void* varg) {
  struct chunk_thread_t *arg = (struct chunk_thread_t*)varg;
  chunk(arg->chunk, arg->x, arg->y, rnd_int(arg->rnd, INT_MAX));
  return 0;
}

static void map_visible_chunks(struct map_t *m, struct camera_t *c) {
  memset(m->visible_chunks, 0, MAX_VISIBLE_CHUNKS * sizeof(int));
    
  int i = 0;
  for (int x = m->camera->chunk_pos_tl.x; x < m->camera->chunk_pos_tr.x + 1; ++x)
    for (int y = m->camera->chunk_pos_tl.y; y < m->camera->chunk_pos_tr.y + 1; ++y)
      for (int j = 0; j < MAX_CHUNKS; ++j) {
        if (m->chunks[j].x == x && m->chunks[j].y == y) {
          m->visible_chunks[i++] = j;
          if (m->camera->chunk_pos.x == x && m->camera->chunk_pos.y == y)
            m->active_chunk = j;
        }
      }
  m->n_visible_chunks = i;
}

bool map(struct map_t *m, struct camera_t *c, struct surface_t *b) {
  prng(&m->seed, 0);
  m->camera = c;
  m->tiles = b;
  m->last_chunk_pos = c->chunk_pos;
  
  if (sqlite3_open(":memory:", &m->db) != SQLITE_OK) {
    sqlite3_close(m->db);
    return false;
  }
  
  if (!exec_sql(m->db, create_db)) {
    sqlite3_close(m->db);
    return false;
  }
  
  thrd_t threads[MAX_CHUNKS];
  struct chunk_thread_t thread_args[MAX_CHUNKS];
  for (int x = -1, n = 0; x < 2; ++x)
    for (int y = -1; y < 2; ++y, ++n) {
      thread_args[n] = (struct chunk_thread_t){
        .chunk = &m->chunks[n],
        .x = x,
        .y = y,
        .rnd = &m->seed
      };
      thrd_create(&threads[n], chunk_thread, &thread_args[n]);
    }
  for (int i = 0; i < MAX_CHUNKS; ++i)
    thrd_join(threads[i], NULL);
  map_visible_chunks(m, c);
  
  return true;
}

void map_update(struct map_t *m, float d) {
  if (!vec2_cmp(m->last_chunk_pos, m->camera->chunk_pos)) {
    switch (bearing(m->last_chunk_pos.x, m->last_chunk_pos.y, m->camera->chunk_pos.x, m->camera->chunk_pos.y)) {
      case 0: // E
        for (int x = m->last_chunk_pos.x - 1, y = m->last_chunk_pos.y - 1; y < m->last_chunk_pos.y + 2; ++y)
          map_deload_chunk(m, x, y);
        for (int x = m->camera->chunk_pos.x + 1, y = m->camera->chunk_pos.y - 1; y < m->camera->chunk_pos.y + 2; ++y)
          map_load_chunk(m, x, y);
        break;
      case 1: // SE
        for (int x = m->last_chunk_pos.x - 1, y = m->last_chunk_pos.y - 1; x < m->last_chunk_pos.x + 2; ++x)
          map_deload_chunk(m, x, y);
        for (int x = m->last_chunk_pos.x - 1, y = m->last_chunk_pos.y; y < m->last_chunk_pos.y + 2; ++y)
          map_deload_chunk(m, x, y);
        for (int x = m->camera->chunk_pos.x + 1,  y = m->camera->chunk_pos.y - 1; y < m->camera->chunk_pos.y + 2; ++y)
          map_load_chunk(m, x, y);
        for (int x = m->camera->chunk_pos.x - 1, y = m->camera->chunk_pos.y + 1; x < m->camera->chunk_pos.x + 1; ++x)
          map_load_chunk(m, x, y);
        break;
      case 2: // S
        for (int x = m->last_chunk_pos.x - 1, y = m->last_chunk_pos.y - 1; x < m->last_chunk_pos.x + 2; ++x)
          map_deload_chunk(m, x, y);
        for (int x = m->camera->chunk_pos.x - 1, y = m->camera->chunk_pos.y + 1; x < m->camera->chunk_pos.x + 2; ++x)
          map_load_chunk(m, x, y);
        break;
      case 3: // SW
        for (int x = m->last_chunk_pos.x - 1, y = m->last_chunk_pos.y - 1; x < m->last_chunk_pos.x + 2; ++x)
          map_deload_chunk(m, x, y);
        for (int x = m->last_chunk_pos.x + 1, y = m->last_chunk_pos.y; y < m->last_chunk_pos.y + 2; ++y)
          map_deload_chunk(m, x, y);
        for (int x = m->camera->chunk_pos.x - 1, y = m->camera->chunk_pos.y - 1; y < m->camera->chunk_pos.y + 2; ++y)
          map_load_chunk(m, x, y);
        for (int x = m->camera->chunk_pos.x, y = m->camera->chunk_pos.y + 1; x < m->camera->chunk_pos.x + 2; ++x)
          map_load_chunk(m, x, y);
        break;
      case 4: // W
        for (int x = m->last_chunk_pos.x + 1, y = m->last_chunk_pos.y - 1; y < m->last_chunk_pos.y + 2; ++y)
          map_deload_chunk(m, x, y);
        for (int x = m->camera->chunk_pos.x - 1, y = m->camera->chunk_pos.y - 1; y < m->camera->chunk_pos.y + 2; ++y)
          map_load_chunk(m, x, y);
        break;
      case 5: // NW
        for (int x = m->last_chunk_pos.x + 1, y = m->last_chunk_pos.y -1; y < m->last_chunk_pos.y + 2; ++y)
          map_deload_chunk(m, x, y);
        for (int x = m->last_chunk_pos.x - 1, y = m->last_chunk_pos.y + 1; x < m->last_chunk_pos.x + 1; ++x)
          map_deload_chunk(m, x, y);
        for (int x = m->camera->chunk_pos.x - 1, y = m->camera->chunk_pos.y - 1; y < m->camera->chunk_pos.y + 2; ++y)
          map_load_chunk(m, x, y);
        for (int x = m->camera->chunk_pos.x, y = m->camera->chunk_pos.y - 1; x < m->camera->chunk_pos.x + 2; ++x)
          map_load_chunk(m, x, y);
        break;
      case 6: // N
        for (int x = m->last_chunk_pos.x - 1, y = m->last_chunk_pos.y + 1; x < m->last_chunk_pos.x + 2; ++x)
          map_deload_chunk(m, x, y);
        for (int x = m->camera->chunk_pos.x - 1, y = m->camera->chunk_pos.y - 1; x < m->camera->chunk_pos.x + 2; ++x)
          map_load_chunk(m, x, y);
        break;
      case 7: // NE
        for (int x = m->last_chunk_pos.x - 1, y = m->last_chunk_pos.y - 1; y < m->last_chunk_pos.y + 2; ++y)
          map_deload_chunk(m, x, y);
        for (int x = m->last_chunk_pos.x, y = m->last_chunk_pos.y + 1; x < m->last_chunk_pos.x + 2; ++x)
          map_deload_chunk(m, x, y);
        for (int x = m->camera->chunk_pos.x - 1, y = m->camera->chunk_pos.y - 1; x < m->camera->chunk_pos.x + 2; ++x)
          map_load_chunk(m, x, y);
        for (int x = m->camera->chunk_pos.x + 1, y = m->camera->chunk_pos.y; y < m->camera->chunk_pos.y + 2; ++y)
          map_load_chunk(m, x, y);
        break;
    }
    m->last_chunk_pos = m->camera->chunk_pos;
  }
  map_visible_chunks(m, m->camera);
}

void map_draw(struct map_t *m, struct surface_t *s) {
  //                 0, -1
  //          +-----------------+
  //          |        |        |
  //          |    0   |    2   |
  //          |        |        |
  //  -1, 0   |--------+--------|   1, 0
  //          |        |        |
  //          |    1   |    3   |
  //          |        |        |
  //          +-----------------+
  //                  0, 1
  
  switch (m->n_visible_chunks) {
    case 1:
      chunk_draw(&m->chunks[m->active_chunk], vec2_add(m->camera->inner_chunk_pos, vec2_neg(m->camera->render_sz_half)), m->camera->visibility, m->tiles, s);
      break;
    case 2:;
      struct chunk_t *active = &m->chunks[m->active_chunk];
      chunk_draw(active, vec2_add(m->camera->inner_chunk_pos, vec2_neg(m->camera->render_sz_half)), m->camera->visibility, m->tiles, s);
      struct chunk_t *other = &m->chunks[m->visible_chunks[m->active_chunk == m->visible_chunks[0]]];
      chunk_draw(other, vec2_add(m->camera->inner_chunk_pos, other->x == active->x && other->y != active->y ? vec2_new(-m->camera->render_sz_half.x, -m->camera->render_sz_half.y + (m->camera->chunk_corner % 2 ? -CHUNK_H : CHUNK_H)) : vec2_new(-m->camera->render_sz_half.x + (m->camera->chunk_corner < 2 ? CHUNK_W : -CHUNK_W), -m->camera->render_sz_half.y)), m->camera->visibility, m->tiles, s);
      break;
    case 4:;
      vec2 offset = vec2_add(m->camera->inner_chunk_pos, vec2_neg(m->camera->render_sz_half));
      struct chunk_t *origin = &m->chunks[m->active_chunk];
      chunk_draw(origin, offset, m->camera->visibility, m->tiles, s);
      for (int i = 0, j; i < MAX_VISIBLE_CHUNKS; ++i) {
        if ((j = m->visible_chunks[i]) == m->active_chunk)
          continue;
        struct chunk_t *other = &m->chunks[m->visible_chunks[i]];
        vec2 mod = vec2_zero();
        switch (bearing(origin->x, origin->y, other->x, other->y)) {
          case 0: // E
            mod = vec2_new(-CHUNK_W, 0.f);
            break;
          case 1: // SE
            mod = vec2_new(-CHUNK_W, -CHUNK_H);
            break;
          case 2: // S
            mod = vec2_new(0.f, -CHUNK_H);
            break;
          case 3: // SW
            mod = vec2_new(CHUNK_W, -CHUNK_H);
            break;
          case 4: // W
            mod = vec2_new(CHUNK_W, 0.f);
            break;
          case 5: // NW
            mod = vec2_new(CHUNK_W, CHUNK_H);
            break;
          case 6: // N
            mod = vec2_new(0.f, CHUNK_H);
            break;
          case 7: // NE
            mod = vec2_new(-CHUNK_W, CHUNK_H);
            break;
        }
        chunk_draw(other, vec2_add(offset, mod), m->camera->visibility, m->tiles, s);
      }
      break;
  }
}

void map_draw_minimap(struct map_t *m, struct surface_t *s) {
  
}

void map_destroy(struct map_t *m) {
  // write changes
  sqlite3_close(m->db);
}
