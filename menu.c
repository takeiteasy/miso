//
//  menu.c
//  worms
//
//  Created by George Watson on 30/05/2021.
//  Copyright © 2021 George Watson. All rights reserved.
//

#include "engine.h"


void menu_ctor() {
  
}

void menu_dtor() {
  
}

enum state_return menu_tick(float dt) {
  if (app()->keyboard.keys[KB_KEY_SPACE])
    return game;
  return loop;
}

void menu_draw() {
  fill(&app()->buffer, RED);
}
