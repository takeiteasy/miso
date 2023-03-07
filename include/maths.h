//
//  maths.h
//  colony
//
//  Created by George Watson on 08/02/2023.
//

#ifndef maths_h
#define maths_h
#include <math.h>
#include <stdbool.h>

#define MIN(a, b) (a < b ? a : b)
#define MAX(a, b) (a > b ? a : b)
#define CLAMP(n, min, max) (MIN(MAX(n, min), max))

#define FLT_EPSILON .000001f

typedef int Vec2i __attribute__((ext_vector_type(2)));
typedef float Vec2 __attribute__((ext_vector_type(2)));
typedef float Vec4 __attribute__((ext_vector_type(4)));

typedef Vec2 Position;

typedef struct {
    Vec2 pos;
    Vec2 size;
} Rect;

bool DoRectsCollide(Rect a, Rect b);
float DistanceBetween(Vec2 a, Vec2 b);
Vec2 MoveTowards(Vec2 position, Vec2 target, float speed);
Rect ScaleBy(Rect rect, float scale);

#endif /* maths_h */
