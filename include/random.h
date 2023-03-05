//
//  random.h
//  colony
//
//  Created by George Watson on 16/02/2023.
//

#ifndef random_h
#define random_h
#include <time.h>
#include <stddef.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>

#define _PRNG_RAND_SSIZE ((UINT16_C(1))<<6)
typedef struct {
    uint64_t seed;
    uint64_t s[_PRNG_RAND_SSIZE]; // Lags
    uint_fast16_t i; // Location of the current lag
    uint_fast16_t c; // Exhaustion count
} Random;

Random NewRandom(uint64_t seed);
float RandomFloat(Random *r);

float Perlin(float x, float y, float z);
unsigned char* PerlinFBM(int w, int h, float xoff, float yoff, float z, float scale, float lacunarity, float gain, int octaves);

#endif /* random_h */
