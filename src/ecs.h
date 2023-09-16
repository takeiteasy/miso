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

typedef enum {
    EcsEntityType    = 0,
    EcsComponentType = (1 << 0),
    EcsSystemType    = (1 << 1),
} EntityFlag;

typedef struct EcsWorld* EcsWorld;

EcsEntity EcsNewEntity(EcsWorld world);
int EcsIsValid(EcsWorld world, EcsEntity e);
int EcsHas(EcsWorld world, EcsEntity entity, EcsEntity component);
EcsEntity EcsNewComponent(EcsWorld world, size_t sizeOfComponent);

void LuaLoadEcs(lua_State *L);

#endif /* ecs_h */
