//
//  ecs.c
//  misoEngine
//
//  Created by George Watson on 15/09/2023.
//

#define LUA_IMPL
#include "ecs.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#if defined(DEBUG)
#include <stdio.h>
#define ASSERT(X) \
    do {                                                                                       \
        if (!(X)) {                                                                            \
            fprintf(stderr, "ERROR! Assertion hit! %s:%s:%d\n", __FILE__, __func__, __LINE__); \
            assert(0);                                                                         \
        }                                                                                      \
    } while(0)
#define ECS_ASSERT(X, Y, V)                                                                    \
    do {                                                                                       \
        if (!(X)) {                                                                            \
            fprintf(stderr, "ERROR! Assertion hit! %s:%s:%d\n", __FILE__, __func__, __LINE__); \
            Dump##Y(V);                                                                        \
            assert(0);                                                                         \
        }                                                                                      \
    } while(0)
#else
#define ECS_ASSERT(X, _, __) assert(X)
#define ASSERT(X) assert(X)
#endif

#define SAFE_FREE(X)    \
    do                  \
    {                   \
        if ((X))        \
        {               \
            free((X));  \
            (X) = NULL; \
        }               \
    } while (0)

const uint64_t EcsNil = 0xFFFFFFFFull;
const EcsEntity EcsNilEntity = { .id = EcsNil };

typedef struct {
    EcsEntity *sparse;
    EcsEntity *dense;
    size_t sizeOfSparse;
    size_t sizeOfDense;
} EcsSparse;

typedef struct {
    EcsEntity componentId;
    void *data;
    size_t sizeOfData;
    size_t sizeOfComponent;
    EcsSparse *sparse;
} EcsStorage;

#if defined(DEBUG)
static void DumpEntity(EcsEntity e) {
    printf("(%llx: %d, %d, %d)\n", e.id, e.parts.id, e.parts.version, e.parts.flag);
}

static void DumpSparse(EcsSparse *sparse) {
    printf("*** DUMP SPARSE ***\n");
    printf("sizeOfSparse: %zu, sizeOfDense: %zu\n", sparse->sizeOfSparse, sparse->sizeOfDense);
    printf("Sparse Contents:\n");
    for (int i = 0; i < sparse->sizeOfSparse; i++)
        DumpEntity(sparse->sparse[i]);
    printf("Dense Contents:\n");
    for (int i = 0; i < sparse->sizeOfDense; i++)
        DumpEntity(sparse->dense[i]);
    printf("*** END SPARSE DUMP ***\n");
}

static void DumpStorage(EcsStorage *storage) {
    printf("*** DUMP STORAGE ***\n");
    printf("componentId: %u, sizeOfData: %zu, sizeOfComponent: %zu\n",
           storage->componentId.parts.id, storage->sizeOfData, storage->sizeOfComponent);
    DumpSparse(storage->sparse);
    printf("*** END STORAGE DUMP ***\n");
}
#endif

static EcsSparse* NewSparse(void) {
    EcsSparse *result = malloc(sizeof(EcsSparse));
    *result = (EcsSparse){0};
    return result;
}

static void DeleteSparse(EcsSparse **p) {
    EcsSparse *_p = *p;
    if (!p || !_p)
        return;
    SAFE_FREE(_p->sparse);
    SAFE_FREE(_p->dense);
    SAFE_FREE(_p);
    *p = NULL;
}

static int SparseHas(EcsSparse *sparse, EcsEntity e) {
    ASSERT(sparse);
    uint32_t id = e.parts.id;
    ASSERT(id != EcsNil);
    return (id < sparse->sizeOfSparse) && (sparse->sparse[id].parts.id != EcsNil);
}

static void SparseEmplace(EcsSparse *sparse, EcsEntity e) {
    ASSERT(sparse);
    uint32_t id = e.parts.id;
    ASSERT(id != EcsNil);
    if (id >= sparse->sizeOfSparse) {
        const size_t newSize = id + 1;
        sparse->sparse = realloc(sparse->sparse, newSize * sizeof * sparse->sparse);
        for (size_t i = sparse->sizeOfSparse; i < newSize; i++)
            sparse->sparse[i] = EcsNilEntity;
        sparse->sizeOfSparse = newSize;
    }
    sparse->sparse[id] = (EcsEntity) { .parts = { .id = (uint32_t)sparse->sizeOfDense } };
    sparse->dense = realloc(sparse->dense, (sparse->sizeOfDense + 1) * sizeof * sparse->dense);
    sparse->dense[sparse->sizeOfDense++] = e;
}

static size_t SparseRemove(EcsSparse *sparse, EcsEntity e) {
    ASSERT(sparse);
    ECS_ASSERT(SparseHas(sparse, e), Sparse, sparse);
    
    const uint32_t id = e.parts.id;
    uint32_t pos = sparse->sparse[id].parts.id;
    EcsEntity other = sparse->dense[sparse->sizeOfDense-1];
    
    sparse->sparse[other.parts.id] = (EcsEntity) { .parts = { .id = pos } };
    sparse->dense[pos] = other;
    sparse->sparse[id] = EcsNilEntity;
    sparse->dense = realloc(sparse->dense, --sparse->sizeOfDense * sizeof * sparse->dense);
    
    return pos;
}

static size_t SparseAt(EcsSparse *sparse, EcsEntity e) {
    ASSERT(sparse);
    uint32_t id = e.parts.id;
    ASSERT(id != EcsNil);
    return sparse->sparse[id].parts.id;
}

static EcsStorage* NewStorage(EcsEntity id, size_t sz) {
    EcsStorage *result = malloc(sizeof(EcsStorage));
    *result = (EcsStorage) {
        .componentId = id,
        .sizeOfComponent = sz,
        .sizeOfData = 0,
        .data = NULL,
        .sparse = NewSparse()
    };
    return result;
}

static void DeleteStorage(EcsStorage **p) {
    EcsStorage *_p = *p;
    if (!p || !_p)
        return;
    DeleteSparse(&_p->sparse);
    *p = NULL;
}

static int StorageHas(EcsStorage *storage, EcsEntity e) {
    ASSERT(storage);
    ASSERT(e.parts.id != EcsNil);
    return SparseHas(storage->sparse, e);
}

static void* StorageEmplace(EcsStorage *storage, EcsEntity e) {
    ASSERT(storage);
    storage->data = realloc(storage->data, (storage->sizeOfData + 1) * sizeof(char) * storage->sizeOfComponent);
    storage->sizeOfData++;
    void *result = &((char*)storage->data)[(storage->sizeOfData - 1) * sizeof(char) * storage->sizeOfComponent];
    SparseEmplace(storage->sparse, e);
    return result;
}

static void StorageRemove(EcsStorage *storage, EcsEntity e) {
    ASSERT(storage);
    size_t pos = SparseRemove(storage->sparse, e);
    memmove(&((char*)storage->data)[pos * sizeof(char) * storage->sizeOfComponent],
            &((char*)storage->data)[(storage->sizeOfData - 1) * sizeof(char) * storage->sizeOfComponent],
            storage->sizeOfComponent);
    storage->data = realloc(storage->data, --storage->sizeOfData * sizeof(char) * storage->sizeOfComponent);
}

static void* StorageAt(EcsStorage *storage, size_t pos) {
    ASSERT(storage);
    ECS_ASSERT(pos < storage->sizeOfData, Storage, storage);
    return &((char*)storage->data)[pos * sizeof(char) * storage->sizeOfComponent];
}

static void* StorageGet(EcsStorage *storage, EcsEntity e) {
    ASSERT(storage);
    ASSERT(e.parts.id != EcsNil);
    return StorageAt(storage, SparseAt(storage->sparse, e));
}

struct EcsWorld {
    EcsStorage **storages;
    size_t sizeOfStorages;
    EcsEntity *entities;
    size_t sizeOfEntities;
    uint32_t *recyclable;
    size_t sizeOfRecyclable;
    uint32_t nextAvailableId;
};

static EcsEntity EcsNewEntityType(EcsWorld world, uint8_t type) {
    ASSERT(world);
    if (world->sizeOfRecyclable) {
        uint32_t idx = world->recyclable[world->sizeOfRecyclable-1];
        EcsEntity e = world->entities[idx];
        EcsEntity new = {
            .parts = {
                .id = e.parts.id,
                .version = e.parts.version,
                .unused = 0,
                .flag = type
            }
        };
        world->entities[idx] = new;
        world->recyclable = realloc(world->recyclable, --world->sizeOfRecyclable * sizeof(uint32_t));
        return new;
    } else {
        world->entities = realloc(world->entities, ++world->sizeOfEntities * sizeof(EcsEntity));
        EcsEntity e = {
            .parts = {
                .id = (uint32_t)world->sizeOfEntities-1,
                .version = 0,
                .unused = 0,
                .flag = type
            }
        };
        world->entities[world->sizeOfEntities-1] = e;
        return e;
    }
}

EcsEntity EcsNewEntity(EcsWorld world) {
    return EcsNewEntityType(world, EcsEntityType);
}

static EcsStorage* EcsFind(EcsWorld world, EcsEntity e) {
    for (int i = 0; i < world->sizeOfStorages; i++)
        if (world->storages[i]->componentId.parts.id == e.parts.id)
            return world->storages[i];
    return NULL;
}

static EcsStorage* EcsAssure(EcsWorld world, EcsEntity componentId, size_t sizeOfComponent) {
    EcsStorage *found = EcsFind(world, componentId);
    if (found)
        return found;
    EcsStorage *new = NewStorage(componentId, sizeOfComponent);
    world->storages = realloc(world->storages, (world->sizeOfStorages + 1) * sizeof * world->storages);
    world->storages[world->sizeOfStorages++] = new;
    return new;
}

EcsEntity EcsNewComponent(EcsWorld world, size_t sizeOfComponent) {
    EcsEntity e = EcsNewEntityType(world, EcsComponentType);
    return EcsAssure(world, e, sizeOfComponent) ? e : EcsNilEntity;
}

int EcsIsValid(EcsWorld world, EcsEntity e) {
    ASSERT(world);
    uint32_t id = e.parts.id;
    return id < world->sizeOfEntities && world->entities[id].parts.id == e.parts.id;
}

int EcsHas(EcsWorld world, EcsEntity entity, EcsEntity component) {
    ASSERT(world);
    ECS_ASSERT(EcsIsValid(world, entity), Entity, entity);
    ECS_ASSERT(EcsIsValid(world, component), Entity, component);
    return StorageHas(EcsFind(world, component), entity);
}

typedef struct {
    EcsWorld world;
} LuaWorld;

static int EcsNewWorldL(lua_State *L) {
    LuaWorld *result = (LuaWorld*)lua_newuserdata(L, sizeof(LuaWorld));
    result->world = malloc(sizeof(struct EcsWorld));
    result->world->nextAvailableId = EcsNil;
    luaL_getmetatable(L, "World");
    lua_setmetatable(L, -2);
    return 1;
}

static int EcsNewEntityL(lua_State *L) {
    LuaWorld *worldL = (LuaWorld*)luaL_checkudata(L, 1, "World");
    EcsEntity e = EcsNewEntity(worldL->world);
    lua_pushinteger(L, e.id);
    return 1;
}

static int EcsDeleteWorldL(lua_State *L) {
    LuaWorld *worldL = (LuaWorld*)luaL_checkudata(L, 1, "World");
    if (worldL->world->storages) {
        for (int i = 0; i < worldL->world->sizeOfStorages; i++)
            DeleteStorage(&worldL->world->storages[i]);
        free(worldL->world->storages);
        worldL->world->storages = NULL;
    }
    SAFE_FREE(worldL->world->entities);
    SAFE_FREE(worldL->world->recyclable);
    SAFE_FREE(worldL->world);
    return 0;
}

static const struct luaL_Reg EcsMethods[] = {
    {"createEntity", EcsNewEntityL},
    {NULL, NULL}
};

static const struct luaL_Reg EcsFunctions[] = {
    {"new", EcsNewWorldL},
    {NULL, NULL}
};

static int luaopen_Ecs(lua_State *L) {
    luaL_newlib(L, EcsFunctions);
    
    luaL_newmetatable(L, "World");
    luaL_newlib(L, EcsMethods);
    lua_setfield(L, -2, "__index");
    
    lua_pushstring(L, "__gc");
    lua_pushcfunction(L, EcsDeleteWorldL);
    lua_settable(L, -3);
    
    lua_pop(L, 1);
    return 1;
}

void LuaLoadEcs(lua_State *L) {
    luaL_requiref(L, "Ecs", &luaopen_Ecs, 1);
    lua_pop(L, 1);
}
