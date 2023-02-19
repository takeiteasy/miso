//
//  random.c
//  colony
//
//  Created by George Watson on 16/02/2023.
//

#include "random.h"

#define _PRNG_LAG1 (UINT16_C(24))
#define _PRNG_LAG2 (UINT16_C(55))
#define _PRNG_RAND_SMASK (_PRNG_RAND_SSIZE-1)
#define _PRNG_RAND_EXHAUST_LIMIT _PRNG_LAG2
// 10x is a heuristic, it just needs to be large enough to remove correlation
#define _PRNG_RAND_REFILL_COUNT ((_PRNG_LAG2*10)-_PRNG_RAND_EXHAUST_LIMIT)
#define PRNG_RAND_MAX UINT64_MAX

static int64_t prng_rand(Random *state) {
    uint_fast16_t i = 0, r = 0, new_rands = 0;
    
    if (!state->c) { // Randomness exhausted, run forward to refill
        new_rands += _PRNG_RAND_REFILL_COUNT+1;
        state->c = _PRNG_RAND_EXHAUST_LIMIT-1;
    } else {
        new_rands = 1;
        state->c--;
    }
    
    for (; r<new_rands; r++ ) {
        i = state->i;
        state->s[i&_PRNG_RAND_SMASK] =
        state->s[(i+_PRNG_RAND_SSIZE-_PRNG_LAG1)&_PRNG_RAND_SMASK] +
        state->s[(i+_PRNG_RAND_SSIZE-_PRNG_LAG2)&_PRNG_RAND_SMASK];
        state->i++;
    }
    
    return state->s[i&_PRNG_RAND_SMASK];
}

Random NewRandom(uint64_t seed) {
    if (seed == 0)
        seed = (uint64_t)clock();
    Random state = {
        .seed = seed,
        .c = _PRNG_RAND_EXHAUST_LIMIT,
        .i = 0
    };
    uint_fast16_t i = 1;
    state.s[0] = seed;
    for(; i < _PRNG_RAND_SSIZE; i++)
        state.s[i] = i*(UINT64_C(2147483647)) + seed;
    
    for (i = 0; i < 10000; i++)
        prng_rand(&state);
    return state;
}

float RandomFloat(Random *rng) {
    return prng_rand(rng) / (float)PRNG_RAND_MAX;
}

static const float grad3[][3] = {
    { 1, 1, 0 }, { -1, 1, 0 }, { 1, -1, 0 }, { -1, -1, 0 },
    { 1, 0, 1 }, { -1, 0, 1 }, { 1, 0, -1 }, { -1, 0, -1 },
    { 0, 1, 1 }, { 0, -1, 1 }, { 0, 1, -1 }, { 0, -1, -1 }
};

static const unsigned int perm[] = {
    182, 232, 51, 15, 55, 119, 7, 107, 230, 227, 6, 34, 216, 61, 183, 36,
    40, 134, 74, 45, 157, 78, 81, 114, 145, 9, 209, 189, 147, 58, 126, 0,
    240, 169, 228, 235, 67, 198, 72, 64, 88, 98, 129, 194, 99, 71, 30, 127,
    18, 150, 155, 179, 132, 62, 116, 200, 251, 178, 32, 140, 130, 139, 250, 26,
    151, 203, 106, 123, 53, 255, 75, 254, 86, 234, 223, 19, 199, 244, 241, 1,
    172, 70, 24, 97, 196, 10, 90, 246, 252, 68, 84, 161, 236, 205, 80, 91,
    233, 225, 164, 217, 239, 220, 20, 46, 204, 35, 31, 175, 154, 17, 133, 117,
    73, 224, 125, 65, 77, 173, 3, 2, 242, 221, 120, 218, 56, 190, 166, 11,
    138, 208, 231, 50, 135, 109, 213, 187, 152, 201, 47, 168, 185, 186, 167, 165,
    102, 153, 156, 49, 202, 69, 195, 92, 21, 229, 63, 104, 197, 136, 148, 94,
    171, 93, 59, 149, 23, 144, 160, 57, 76, 141, 96, 158, 163, 219, 237, 113,
    206, 181, 112, 111, 191, 137, 207, 215, 13, 83, 238, 249, 100, 131, 118, 243,
    162, 248, 43, 66, 226, 27, 211, 95, 214, 105, 108, 101, 170, 128, 210, 87,
    38, 44, 174, 188, 176, 39, 14, 143, 159, 16, 124, 222, 33, 247, 37, 245,
    8, 4, 22, 82, 110, 180, 184, 12, 25, 5, 193, 41, 85, 177, 192, 253,
    79, 29, 115, 103, 142, 146, 52, 48, 89, 54, 121, 212, 122, 60, 28, 42,
    
    182, 232, 51, 15, 55, 119, 7, 107, 230, 227, 6, 34, 216, 61, 183, 36,
    40, 134, 74, 45, 157, 78, 81, 114, 145, 9, 209, 189, 147, 58, 126, 0,
    240, 169, 228, 235, 67, 198, 72, 64, 88, 98, 129, 194, 99, 71, 30, 127,
    18, 150, 155, 179, 132, 62, 116, 200, 251, 178, 32, 140, 130, 139, 250, 26,
    151, 203, 106, 123, 53, 255, 75, 254, 86, 234, 223, 19, 199, 244, 241, 1,
    172, 70, 24, 97, 196, 10, 90, 246, 252, 68, 84, 161, 236, 205, 80, 91,
    233, 225, 164, 217, 239, 220, 20, 46, 204, 35, 31, 175, 154, 17, 133, 117,
    73, 224, 125, 65, 77, 173, 3, 2, 242, 221, 120, 218, 56, 190, 166, 11,
    138, 208, 231, 50, 135, 109, 213, 187, 152, 201, 47, 168, 185, 186, 167, 165,
    102, 153, 156, 49, 202, 69, 195, 92, 21, 229, 63, 104, 197, 136, 148, 94,
    171, 93, 59, 149, 23, 144, 160, 57, 76, 141, 96, 158, 163, 219, 237, 113,
    206, 181, 112, 111, 191, 137, 207, 215, 13, 83, 238, 249, 100, 131, 118, 243,
    162, 248, 43, 66, 226, 27, 211, 95, 214, 105, 108, 101, 170, 128, 210, 87,
    38, 44, 174, 188, 176, 39, 14, 143, 159, 16, 124, 222, 33, 247, 37, 245,
    8, 4, 22, 82, 110, 180, 184, 12, 25, 5, 193, 41, 85, 177, 192, 253,
    79, 29, 115, 103, 142, 146, 52, 48, 89, 54, 121, 212, 122, 60, 28, 42
};

static float dot3(const float a[], float x, float y, float z) {
    return a[0]*x + a[1]*y + a[2]*z;
}

static float lerp(float a, float b, float t) {
    return (1 - t) * a + t * b;
}

static float fade(float t) {
    return t * t * t * (t * (t * 6 - 15) + 10);
}

#define FASTFLOOR(x)  (((x) >= 0) ? (int)(x) : (int)(x)-1)

float Perlin(float x, float y, float z) {
    /* Find grid points */
    int gx = FASTFLOOR(x);
    int gy = FASTFLOOR(y);
    int gz = FASTFLOOR(z);
    
    /* Relative coords within grid cell */
    float rx = x - gx;
    float ry = y - gy;
    float rz = z - gz;
    
    /* Wrap cell coords */
    gx = gx & 255;
    gy = gy & 255;
    gz = gz & 255;
    
    /* Calculate gradient indices */
    unsigned int gi[8];
    for (int i = 0; i < 8; i++)
        gi[i] = perm[gx+((i>>2)&1)+perm[gy+((i>>1)&1)+perm[gz+(i&1)]]] % 12;
    
    /* Noise contribution from each corner */
    float n[8];
    for (int i = 0; i < 8; i++)
        n[i] = dot3(grad3[gi[i]], rx - ((i>>2)&1), ry - ((i>>1)&1), rz - (i&1));
    
    /* Fade curves */
    float u = fade(rx);
    float v = fade(ry);
    float w = fade(rz);
    
    /* Interpolate */
    float nx[4];
    for (int i = 0; i < 4; i++)
        nx[i] = lerp(n[i], n[4+i], u);
    
    float nxy[2];
    for (int i = 0; i < 2; i++)
        nxy[i] = lerp(nx[i], nx[2+i], v);
    
    return lerp(nxy[0], nxy[1], w);
}

static float Remap(float value, float from1, float to1, float from2, float to2) {
    return (value - from1) / (to1 - from1) * (to2 - from2) + from2;
}

unsigned char* PerlinFBM(int w, int h, float xoff, float yoff, float scale, float lacunarity, float gain, int octaves, bool fadeOut, Random *rng) {
    float z = RandomFloat(rng);
    float min = FLT_MAX, max = FLT_MIN;
    float *grid = malloc(w * h * sizeof(float));
    for (int x = 0; x < w; ++x)
        for (int y = 0; y < h; ++y) {
            float freq = 2.f,
                  amp  = 1.f,
                  tot  = 0.f,
                  sum  = 0.f;
            for (int i = 0; i < octaves; ++i) {
                sum  += Perlin(((xoff + x) / scale) * freq, ((yoff + y) / scale) * freq, z) * amp;
                tot  += amp;
                freq *= lacunarity;
                amp  *= gain;
            }
            grid[y * w + x] = sum = (sum / tot);
            if (sum < min)
                min = sum;
            if (sum > max)
                max = sum;
        }
    
    unsigned char *result = malloc(w * h * sizeof(unsigned char));
    for (int x = 0; x < w; x++)
        for (int y = 0; y < h; y++) {
            float height = 255.f - (255.f * Remap(grid[y * w + x], min, max, 0, 1.f));
            float grad = fadeOut ? sqrtf(powf(w / 2 - x, 2.f) + powf(h / 2 - y, 2.f)) : 0.f;
            float final = height - grad;
            result[y * w + x] = (unsigned char)final;
        }
    free(grid);
    return result;
}
