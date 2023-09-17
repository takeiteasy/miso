//
//  ecs.h
//  misoEngine
//
//  Created by George Watson on 15/09/2023.
//

#ifndef ecs_h
#define ecs_h
#include <stdint.h>
#include "minilua.h"

typedef union {
    struct {
        uint32_t id;
        uint16_t version;
        uint8_t unused;
        uint8_t flag;
    } parts;
    uint64_t id;
} EcsEntity;

typedef enum EcsType {
    EcsNormal = 0,
    EcsComponent,
    EcsSystem,
} EcsType;

typedef struct EcsWorld* EcsWorld;

void InitEcsWorld(void);
void DestroyEcsWorld(void);
EcsEntity EcsNewEntity(uint8_t type);
int EcsIsEntityValid(EcsEntity e);
int EcsEntityHas(EcsEntity entity, EcsEntity component);
EcsEntity EcsNewComponent(size_t sizeOfComponent);

void LuaLoadEcs(lua_State *L);

#endif /* ecs_h */
