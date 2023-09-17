//
//  ecs.c
//  misoEngine
//
//  Created by George Watson on 15/09/2023.
//


#define LUA_IMPL
#include "ecs.h"
#define LUACSTRUCT_IMPL
#include "luacstruct.h"
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

static struct EcsWorld {
    EcsStorage **storages;
    size_t sizeOfStorages;
    EcsEntity *entities;
    size_t sizeOfEntities;
    uint32_t *recyclable;
    size_t sizeOfRecyclable;
    uint32_t nextAvailableId;
} world;

void InitEcsWorld(void) {
    DestroyEcsWorld();
    world.nextAvailableId = EcsNil;
    // TODO: Initialize built-in defaults
}

void DestroyEcsWorld(void) {
    if (world.storages) {
        for (int i = 0; i < world.sizeOfStorages; i++)
            DeleteStorage(&world.storages[i]);
        free(world.storages);
    }
    SAFE_FREE(world.entities);
    SAFE_FREE(world.recyclable);
    memset(&world, 0, sizeof(struct EcsWorld));
}

EcsEntity EcsNewEntity(uint8_t type) {
    if (world.sizeOfRecyclable) {
        uint32_t idx = world.recyclable[world.sizeOfRecyclable-1];
        EcsEntity e = world.entities[idx];
        EcsEntity new = {
            .parts = {
                .id = e.parts.id,
                .version = e.parts.version,
                .unused = 0,
                .flag = type
            }
        };
        world.entities[idx] = new;
        world.recyclable = realloc(world.recyclable, --world.sizeOfRecyclable * sizeof(uint32_t));
        return new;
    } else {
        world.entities = realloc(world.entities, ++world.sizeOfEntities * sizeof(EcsEntity));
        EcsEntity e = {
            .parts = {
                .id = (uint32_t)world.sizeOfEntities-1,
                .version = 0,
                .unused = 0,
                .flag = type
            }
        };
        world.entities[world.sizeOfEntities-1] = e;
        return e;
    }
}

static EcsStorage* EcsFind(EcsEntity e) {
    for (int i = 0; i < world.sizeOfStorages; i++)
        if (world.storages[i]->componentId.parts.id == e.parts.id)
            return world.storages[i];
    return NULL;
}

static EcsStorage* EcsAssure(EcsEntity componentId, size_t sizeOfComponent) {
    EcsStorage *found = EcsFind(componentId);
    if (found)
        return found;
    EcsStorage *new = NewStorage(componentId, sizeOfComponent);
    world.storages = realloc(world.storages, (world.sizeOfStorages + 1) * sizeof * world.storages);
    world.storages[world.sizeOfStorages++] = new;
    return new;
}

EcsEntity EcsNewComponent(size_t sizeOfComponent) {
    EcsEntity e = EcsNewEntity(EcsComponent);
    return EcsAssure(e, sizeOfComponent) ? e : EcsNilEntity;
}

int EcsIsEntityValid(EcsEntity e) {
    uint32_t id = e.parts.id;
    return id < world.sizeOfEntities && world.entities[id].parts.id == e.parts.id;
}

int EcsEntityHas(EcsEntity entity, EcsEntity component) {
    ECS_ASSERT(EcsIsEntityValid(entity), Entity, entity);
    ECS_ASSERT(EcsIsEntityValid(component), Entity, component);
    return StorageHas(EcsFind(component), entity);
}

static void LuaDumpTable(lua_State* L);

static void PrintStackAt(lua_State *L, int idx) {
    int t = lua_type(L, idx);
    switch (t) {
        case LUA_TSTRING:
            printf("(string): `%s'", lua_tostring(L, idx));
            break;
        case LUA_TBOOLEAN:
            printf("(boolean): %s", lua_toboolean(L, idx) ? "true" : "false");
            break;
        case LUA_TNUMBER:
            printf("(integer): %g",  lua_tonumber(L, idx));
            break;
        case LUA_TTABLE:
            printf("(table):\n");
            lua_settop(L, idx);
            LuaDumpTable(L);
            break;
        default:;
            printf("(%s): %p", lua_typename(L, t), lua_topointer(L, idx));
            break;
    }
}

static void LuaDumpTable(lua_State* L) {
    if (!lua_istable(L, -1))
        luaL_error(L, "Expected a table at the top of the stack");
    
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        if (lua_type(L, -2) == LUA_TSTRING)
            printf("%s", lua_tostring(L, -2));
        else
            PrintStackAt(L, -2);
        if (lua_type(L, -1) == LUA_TTABLE) {
            printf("\n");
            LuaDumpTable(L);
        } else {
            printf(" -- ");
            PrintStackAt(L, -1);
            printf("\n");
        }
        lua_pop(L, 1);
    }
}

static int LuaDumpStack(lua_State* L) {
    printf("--------------- LUA STACK DUMP ---------------\n");
    for (int i = lua_gettop(L); i; --i) {
        PrintStackAt(L, i);
        if (i > 1)
            printf("\n");
    }
    printf("--------------- END STACK DUMP ---------------\n");
    return 0;
}

static int luaEcsResetWorld(lua_State *L) {
    InitEcsWorld();
    return 0;
}

static int luaEcsNewEntity(lua_State *L) {
    EcsType type = EcsNormal;
    switch (lua_gettop(L)) {
        case 1:
            break;
        case 2:;
            assert(lua_isuserdata(L, 2));
            struct luacenum_value *v = luaL_checkudata(L, 2, "luacenumval1");
            type = (EcsType)v->value;
            break;
        default:
            assert(0);
    }
    EcsEntity e = EcsNewEntity((uint8_t)type);
    lua_pushinteger(L, e.id);
    return 1;
}

static int luaEcsNewComponent(lua_State *L) {
    const char *name = lua_tostring(L, 2);
    lua_getglobal(L, name);
    LuaDumpStack(L);
    return 1;
}

static const struct luaL_Reg EcsMethods[] = {
    {NULL, NULL}
};

static const struct luaL_Reg EcsFunctions[] = {
    {"resetWorld", luaEcsResetWorld},
    {"createEntity", luaEcsNewEntity},
    {"createComponent", luaEcsNewComponent},
    {NULL, NULL}
};

static int luaopen_Ecs(lua_State *L) {
    luaL_newlib(L, EcsFunctions);
    luaL_newmetatable(L, "Ecs");
    luaL_newlib(L, EcsMethods);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
    return 1;
}

void LuaLoadEcs(lua_State *L) {
    luaL_requiref(L, "Ecs", &luaopen_Ecs, 1);
    lua_pop(L, 1);
    
    luacs_newenum(L, EcsType);
    luacs_enum_declare_value(L, "Entity",   EcsNormal);
    luacs_enum_declare_value(L, "Component", EcsComponent);
    luacs_enum_declare_value(L, "System",  EcsSystem);
    lua_setglobal(L, "EcsType");
}
