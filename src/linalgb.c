//
//  linalgb.c
//  colony
//
//  Created by George Watson on 15/02/2023.
//

#include "linalgb.h"

bool DoRectsCollide(Rect a, Rect b) {
    return a.pos.x < b.pos.x + b.size.x &&
           a.pos.x + a.size.x > b.pos.x &&
           a.pos.y < b.pos.y + b.size.y &&
           a.size.y + a.pos.y > b.pos.y;
}
