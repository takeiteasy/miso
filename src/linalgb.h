//
//  linalgb.h
//  colony
//
//  Created by George Watson on 08/02/2023.
//

#ifndef linalgb_h
#define linalgb_h
#include <math.h>

#define MIN(a, b) (a < b ? a : b)
#define MAX(a, b) (a > b ? a : b)
#define CLAMP(n, min, max) (MIN(MAX(n, min), max))

typedef int Vec2i __attribute__((ext_vector_type(2)));
typedef float Vec2 __attribute__((ext_vector_type(2)));
typedef float Vec4 __attribute__((ext_vector_type(4)));
typedef Vec2 Position;
typedef Vec2i GridPosition;

typedef struct {
    int w, h;
} Size;

typedef struct {
    Vec2 pos;
    Vec2 size;
} Rect;

typedef struct {
    Vec2 position, texcoord;
    Vec4 color;
} Vertex;

#endif /* linalgb_h */