//
//  random.c
//  worms
//
//  Created by George Watson on 17/06/2021.
//  Copyright © 2021 George Watson. All rights reserved.
//

#include "random.h"

static unsigned rotl(unsigned n, unsigned r) {
  return  (n << r) | (n >> (32 - r));
}

static unsigned rotr(unsigned n, unsigned r) {
  return  (n >> r) | (n << (32 - r));
}

unsigned prng(struct prng_t *r, unsigned s) {
  if (!s)
    s = (unsigned)clock();
  
  for (unsigned i = 0; i < 17; i++) {
    s = s * 2891336453 + 1;
    r->buffer[i] = s;
  }
  
  r->p1 = 0;
  r->p2 = 10;
  return s;
}

unsigned rnd_bits(struct prng_t *r) {
  unsigned result = r->buffer[r->p1] = rotl(r->buffer[r->p2], 13) + rotl(r->buffer[r->p1], 9);
  
  if (--r->p1 < 0) r->p1 = 16;
  if (--r->p2 < 0) r->p2 = 16;
  
  return result;
}

union flt_convert {
  float value;
  unsigned word;
};

float rnd_float(struct prng_t *r) {
  unsigned bits = rnd_bits(r);
  union flt_convert convert;
  convert.word = (bits >> 9) | 0x3f800000;
  return convert.value - 1.0f;
}

union dbl_convert {
  double value;
  unsigned words[2];
};

double rnd_double(struct prng_t *r) {
  unsigned bits = rnd_bits(r);
  union dbl_convert convert;
  convert.words[0] =  bits << 20;
  convert.words[1] = (bits >> 12) | 0x3FF00000;
  return convert.value - 1.0;
}

unsigned rnd_int(struct prng_t *r, int max) {
  return rnd_bits(r) % max;
}

float rnd_float_range(struct prng_t *r, float min, float max) {
  return rnd_float(r) * (max - min) + min;
}

double rnd_double_range(struct prng_t *r, double min, double max) {
  return rnd_double(r) * (max - min) + min;
}

unsigned rnd_int_range(struct prng_t *r, int min, int max) {
  return 0;
}

#define idx(x, y) ((y) * w + (x))

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

float simplex(float x, float y, float z) {
  /* Skew input space */
  float s = (x+y+z)*(1.0/3.0);
  int i = FASTFLOOR(x+s);
  int j = FASTFLOOR(y+s);
  int k = FASTFLOOR(z+s);
  
  /* Unskew */
  float t = (float)(i+j+k)*(1.0/6.0);
  float gx0 = i-t;
  float gy0 = j-t;
  float gz0 = k-t;
  float x0 = x-gx0;
  float y0 = y-gy0;
  float z0 = z-gz0;
  
  /* Determine simplex */
  int i1, j1, k1;
  int i2, j2, k2;
  
  if (x0 >= y0) {
    if (y0 >= z0) {
      i1 = 1; j1 = 0; k1 = 0;
      i2 = 1; j2 = 1; k2 = 0;
    } else if (x0 >= z0) {
      i1 = 1; j1 = 0; k1 = 0;
      i2 = 1; j2 = 0; k2 = 1;
    } else {
      i1 = 0; j1 = 0; k1 = 1;
      i2 = 1; j2 = 0; k2 = 1;
    }
  } else {
    if (y0 < z0) {
      i1 = 0; j1 = 0; k1 = 1;
      i2 = 0; j2 = 1; k2 = 1;
    } else if (x0 < z0) {
      i1 = 0; j1 = 1; k1 = 0;
      i2 = 0; j2 = 1; k2 = 1;
    } else {
      i1 = 0; j1 = 1; k1 = 0;
      i2 = 1; j2 = 1; k2 = 0;
    }
  }
  
  /* Calculate offsets in x,y,z coords */
  float x1 = x0 - i1 + (1.0/6.0);
  float y1 = y0 - j1 + (1.0/6.0);
  float z1 = z0 - k1 + (1.0/6.0);
  float x2 = x0 - i2 + 2.0*(1.0/6.0);
  float y2 = y0 - j2 + 2.0*(1.0/6.0);
  float z2 = z0 - k2 + 2.0*(1.0/6.0);
  float x3 = x0 - 1.0 + 3.0*(1.0/6.0);
  float y3 = y0 - 1.0 + 3.0*(1.0/6.0);
  float z3 = z0 - 1.0 + 3.0*(1.0/6.0);
  
  int ii = i % 256;
  int jj = j % 256;
  int kk = k % 256;
  
  /* Calculate gradient incides */
  int gi0 = perm[ii+perm[jj+perm[kk]]] % 12;
  int gi1 = perm[ii+i1+perm[jj+j1+perm[kk+k1]]] % 12;
  int gi2 = perm[ii+i2+perm[jj+j2+perm[kk+k2]]] % 12;
  int gi3 = perm[ii+1+perm[jj+1+perm[kk+1]]] % 12;
  
  /* Calculate contributions */
  float n0, n1, n2, n3;
  
  float t0 = 0.6 - x0*x0 - y0*y0 - z0*z0;
  if (t0 < 0) n0 = 0.0;
  else {
    t0 *= t0;
    n0 = t0 * t0 * dot3(grad3[gi0], x0, y0, z0);
  }
  
  float t1 = 0.6 - x1*x1 - y1*y1 - z1*z1;
  if (t1 < 0) n1 = 0.0;
  else {
    t1 *= t1;
    n1 = t1 * t1 * dot3(grad3[gi1], x1, y1, z1);
  }
  
  float t2 = 0.6 - x2*x2 - y2*y2 - z2*z2;
  if (t2 < 0) n2 = 0.0;
  else {
    t2 *= t2;
    n2 = t2 * t2 * dot3(grad3[gi2], x2, y2, z2);
  }
  
  float t3 = 0.6 - x3*x3 - y3*y3 - z3*z3;
  if (t3 < 0) n3 = 0.0;
  else {
    t3 *= t3;
    n3 = t3 * t3 * dot3(grad3[gi3], x3, y3, z3);
  }
  
  /* Return scaled sum of contributions */
  return 32.0*(n0 + n1 + n2 + n3);
}

float perlin(float x, float y, float z) {
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

static bool in_valid_space(vec2 p, bool circle) {
  return circle ? powf(p.x - .5f, 2.f) + powf(p.y - .5f, 2.f) <= .25f : p.x >= 0 && p.y >= 0 && p.x <= 1 && p.y <= 1;
}

static vec2 image_to_grid(vec2 p, float cs) {
  return vec2_new((int)(p.x / cs), (int)(p.y / cs));
}

#define ADD_POINT(p) \
  do { \
    stb_sb_push(sample, (p)); \
    stb_sb_push(active, (p)); \
    vec2 _p = image_to_grid((p), cell_sz); \
    grid[(int)_p.y * w + (int)_p.x] = stb_sb_count(sample); \
  } while(0);

vec2* poisson(struct prng_t *r, int n, int k, bool circle) {
  vec2 *sample = NULL, *active = NULL;
  if (circle)
    n = (int)(M_PI_4 * (float)n);
  float m_dist = sqrtf((float)n) / (float)n;
  float m_dist_sqr = m_dist * m_dist;
  float cell_sz = m_dist / sqrt(2.f);
  int w = (int)ceil(1.f / cell_sz );
  int h = (int)ceil(1.f / cell_sz );
  size_t grid_sz = w * h * sizeof(int);
  int *grid = malloc(grid_sz);
  memset(grid, 0, grid_sz);
  
  vec2 origin;
  do {
    origin = vec2_new(rnd_float(r), rnd_float(r));
  } while (!in_valid_space(origin, circle));
  ADD_POINT(origin);
  
  while (stb_sb_count(active) && stb_sb_count(sample) < n) {
    int idx = rand() % stb_sb_count(active);
    vec2 p = active[idx];
    stb_sb_pop(active, idx);
    
    for (int i = 0; i < k; ++i) {
      float radius = m_dist * (rnd_float(r) + 1.f);
      float angle  = 2 * M_PI * rnd_float(r);
      vec2 np = vec2_new(p.x + radius * cos(angle),
                         p.y + radius * sin(angle));
      if (!in_valid_space(np, circle))
        continue;
      
      bool in_neighbourhood = false;
      vec2 g = image_to_grid(np, cell_sz);
      vec2 g_min = vec2_new(max(g.x - 2, 0),
                            max(g.y - 2, 0));
      vec2 g_max = vec2_new(min(g.x + 2, w - 1),
                            min(g.y + 2, h - 1));
      for (int x = g_min.x; x <= g_max.x; ++x) {
        for (int y = g_min.y; y <= g_max.y; ++y) {
          int idx = grid[y * w + x] - 1;
          if (idx < 0)
            continue;
          vec2 P = sample[idx];
          
          if (vec2_dist_sqrd(P, np) < m_dist_sqr) {
            in_neighbourhood = true;
            break;
          }
        }
        if (in_neighbourhood)
          break;
      }
      
      if (!in_neighbourhood)
        ADD_POINT(np);
    }
  }
  
  free(grid);
  stb_sb_free(active);
  return sample;
}

//static int cmp(const void* a, const void* b) {
//  float fa = *(const float*) a;
//  float fb = *(const float*) b;
//  return (fa > fb) - (fa < fb);
//}
//
//static float remap(float x, float in_min, float in_max, float out_min, float out_max)
//{
//  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
//}

float* worley(struct prng_t *r, int w, int h, int n, bool dist) {
//  vec2 *points = poisson(n, 30);
//  int n_points = stb_sb_count(points);
//  for (int x = 0; x < w; ++x) {
//    for (int y = 0; y < h; ++y) {
//      float d[n_points];
//      vec2 p = vec2_div(vec2_new(x, y), 128);
//      for (int i = 0; i < n_points; ++i)
//        d[i] = vec2_dist(p, points[i]);
//      qsort((void*)d, n_points, sizeof(float), cmp);
//        //      printf("%f\n", d[1] - d[0]);
//      if (d[1] - d[0] < .02)
//        m->tiles[idx(x, y)] = WATER;
//    }
//  }
  return NULL;
}
