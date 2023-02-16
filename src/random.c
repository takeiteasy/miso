//
//  random.c
//  colony
//
//  Created by George Watson on 16/02/2023.
//

#include "random.h"

Random NewRandom(unsigned int s) {
    Random result = {
        .seed = s ? s : (unsigned int)clock(),
        .p1 = 0,
        .p2 = 10
    };
    for (int i = 0; i < 17; i++) {
        s = s * 0xAC564B05 + 1;
        result.buffer[i] = s;
    }
    return result;
}

#define ROTL(N, R) (((N) << (R)) | ((N) >> (32 - (R))))

unsigned int RandomBits(Random *r) {
    unsigned int result = r->buffer[r->p1] = ROTL(r->buffer[r->p2], 13) + ROTL(r->buffer[r->p1], 9);

    if (--r->p1 < 0)
        r->p1 = 16;
    if (--r->p2 < 0)
        r->p2 = 16;

    return result;
}

float RandomFloat(Random *r) {
    union {
        float value;
        unsigned int word;
    } convert = {
        .word = (RandomBits(r) >> 9) | 0x3f800000};
    return convert.value - 1.0f;
}

double RandomDouble(Random *r) {
    unsigned int bits = RandomBits(r);
    union {
        double value;
        unsigned int words[2];
    } convert = {
        .words = {
            bits << 20,
            (bits >> 12) | 0x3FF00000}};
    return convert.value - 1.0;
}

int RandomInt(Random *r, int max) {
    return RandomBits(r) % max;
}

float RandomFloatRange(Random *r, float min, float max) {
    return RandomFloat(r) * (max - min) + min;
}

double RandomDoubleRange(Random *r, double min, double max) {
    return RandomDouble(r) * (max - min) + min;
}

int RandomIntRange(Random *r, int min, int max) {
    return RandomBits(r) % (max + 1 - min) + min;
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
  for (int i = 0; i < 8; i++) gi[i] = perm[gx+((i>>2)&1)+perm[gy+((i>>1)&1)+perm[gz+(i&1)]]] % 12;
  
  /* Noise contribution from each corner */
  float n[8];
  for (int i = 0; i < 8; i++) n[i] = dot3(grad3[gi[i]], rx - ((i>>2)&1), ry - ((i>>1)&1), rz - (i&1));
  
  /* Fade curves */
  float u = fade(rx);
  float v = fade(ry);
  float w = fade(rz);
  
  /* Interpolate */
  float nx[4];
  for (int i = 0; i < 4; i++) nx[i] = lerp(n[i], n[4+i], u);
  
  float nxy[2];
  for (int i = 0; i < 2; i++) nxy[i] = lerp(nx[i], nx[2+i], v);
  
  return lerp(nxy[0], nxy[1], w);
}
