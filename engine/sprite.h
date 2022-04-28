//
//  sprite.h
//  worms
//
//  Created by George Watson on 29/05/2021.
//  Copyright © 2021 George Watson. All rights reserved.
//

#ifndef sprite_h
#define sprite_h
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "graphics.h"
#include "json.h"

struct frame_t {
  int x, y, w, h, duration;
};

struct atlas_t {
  struct frame_t *frames;
  int n_frames;
};

struct keyframe_t {
  char *name;
  int from, to;
};

struct animation_t {
  struct keyframe_t *frames;
  int n_frames;
};

struct sprite_t {
  struct surface_t buffer;
  int w, h;
  struct atlas_t atlas;
  struct animation_t animation;
  struct keyframe_t *frame;
  int keyframe;
  float timer;
  bool animated;
};

bool sprite(struct sprite_t *s, const char *p);
bool spirte_frame(struct sprite_t *s, const char *f);
void sprite_update(struct sprite_t *s, float dt);
void sprite_draw(struct sprite_t *s, int x, int y, struct surface_t *b);
void sprite_destroy(struct sprite_t *s);

#endif /* sprite_h */
