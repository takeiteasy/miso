//
//  maths.c
//  colony
//
//  Created by George Watson on 15/02/2023.
//

#include "maths.h"

bool DoRectsCollide(Rect a, Rect b) {
    return a.pos.x < b.pos.x + b.size.x &&
           a.pos.x + a.size.x > b.pos.x &&
           a.pos.y < b.pos.y + b.size.y &&
           a.size.y + a.pos.y > b.pos.y;
}

float DistanceBetween(Vec2 a, Vec2 b) {
    return sqrtf((a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y));
}

Vec2 MoveTowards(Vec2 position, Vec2 target, float speed) {
    float dist = DistanceBetween(position, target);
    if (dist <= FLT_EPSILON)
        return position;
    float min_step = MAX(0, dist - 100.f);
    return position + ((target - position) / dist) * (min_step + ((dist - FLT_EPSILON) - min_step) * speed);
}

Rect ScaleBy(Rect rect, float scale) {
    Vec2 size = rect.size * scale;
    return (Rect) {
        .pos = rect.pos - (rect.size / 2) - (size / 2),
        .size = size
    };
}
