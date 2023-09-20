//
//  ecs.h
//  misoEngine
//
//  Created by George Watson on 15/09/2023.
//

#ifndef ecs_h
#define ecs_h
#include "lua.h"
#include "hashmap.h"

void InitEcsWorld(void);
void EcsStep(void);
void DestroyEcsWorld(void);
void LuaLoadEcs(lua_State *L);

#endif /* ecs_h */
