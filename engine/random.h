//
//  random.h
//  worms
//
//  Created by George Watson on 17/06/2021.
//  Copyright © 2021 George Watson. All rights reserved.
//

#ifndef random_h
#define random_h
#include "linalgb.h"
#include "random.h"
#include "stretchy_buffer.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

struct prng_t {
  int p1, p2;
  unsigned buffer[17];
};

unsigned prng(struct prng_t *r, unsigned s);
unsigned rnd_bits(struct prng_t *r);
float rnd_float(struct prng_t *r);
double rnd_double(struct prng_t *r);
unsigned rnd_int(struct prng_t *r, int max);
float rnd_float_range(struct prng_t *r, float min, float max);
double rnd_double_range(struct prng_t *r, double min, double max);
unsigned rnd_int_range(struct prng_t *r, int min, int max);

typedef float (*noise_fn)(float x, float y, float z);
float simplex(float x, float y, float z);
float perlin(float x, float y, float z);

vec2* poisson(struct prng_t *r, int n, int k, bool circle);
float* worley(struct prng_t *r, int w, int h, int n, bool dist);

#endif /* random_h */
