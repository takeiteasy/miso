//
//  ecs.c
//  misoEngine
//
//  Created by George Watson on 15/09/2023.
//


#define LUA_IMPL
#define LUACSTRUCT_IMPL
#define HASHMAP_IMPL
#include "ecs.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
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

typedef union {
    struct {
        uint32_t id;
        uint16_t version;
        uint8_t unused;
        uint8_t flag;
    } parts;
    uint64_t id;
} EcsEntity;

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

typedef enum EcsType {
    EcsNormal = 0,
    EcsComponent,
    EcsSystem,
} EcsType;

typedef struct {
    union {
        lua_Integer integer;
        lua_Number number;
        const char *string;
    } data;
    int type;
} LuaType;

typedef struct {
    struct hashmap *map;
    EcsEntity id;
    const char *name;
} LuaComponent;

typedef struct {
    LuaType value;
    const char *name;
} LuaComponentMember;

typedef struct EcsEntity {
    EcsEntity id;
} LuaEntity;

static struct EcsWorld {
    EcsStorage **storages;
    size_t sizeOfStorages;
    EcsEntity *entities;
    size_t sizeOfEntities;
    uint32_t *recyclable;
    size_t sizeOfRecyclable;
    uint32_t nextAvailableId;
    struct hashmap *components;
} world;

static int ComponentCompare(const void *a, const void *b, void *udata) {
    return strcmp(((LuaComponent*)a)->name, ((LuaComponent*)b)->name);
}

static uint64_t ComponentHash(const void *item, uint64_t seed0, uint64_t seed1) {
    LuaComponent *entity = (LuaComponent*)item;
    return hashmap_sip(entity->name, strlen(entity->name), seed0, seed1);
}

static void ComponentFree(void *item) {
    LuaComponent *component = (LuaComponent*)item;
    hashmap_free(component->map);
    free(item);
}

void InitEcsWorld(void) {
    DestroyEcsWorld();
    world.nextAvailableId = EcsNil;
    world.components = hashmap_new(sizeof(LuaComponent), 0, 0, 0, ComponentHash, ComponentCompare, ComponentFree, NULL);
    assert(world.components);
    // TODO: Initialize built-in defaults
}

void EcsStep(void) {
    // TODO: Implement world step
}

void DestroyEcsWorld(void) {
    if (world.storages) {
        for (int i = 0; i < world.sizeOfStorages; i++)
            DeleteStorage(&world.storages[i]);
        free(world.storages);
    }
    SAFE_FREE(world.entities);
    SAFE_FREE(world.recyclable);
    if (world.components)
        hashmap_free(world.components);
    memset(&world, 0, sizeof(struct EcsWorld));
}

static EcsEntity EcsNewEntity(uint8_t type) {
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

static int EcsIsEntityValid(EcsEntity e) {
    uint32_t id = e.parts.id;
    return id < world.sizeOfEntities && world.entities[id].parts.id == e.parts.id;
}

static int EcsEntityHas(EcsEntity entity, EcsEntity component) {
    ECS_ASSERT(EcsIsEntityValid(entity), Entity, entity);
    ECS_ASSERT(EcsIsEntityValid(component), Entity, component);
    return StorageHas(EcsFind(component), entity);
}

static int LuaDumpTable(lua_State* L);

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
            if (lua_isnumber(L, idx))
                printf("(number): %g",  lua_tonumber(L, idx));
            else
                printf("(integer): %lld",  lua_tointeger(L, idx));
            break;
        case LUA_TTABLE:
            printf("(table):\n");
            lua_settop(L, idx);
            LuaDumpTable(L);
            break;
        default:;
            printf("(%s): %p\n", lua_typename(L, t), lua_topointer(L, idx));
            break;
    }
}

static int LuaDumpTable(lua_State* L) {
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
    return 0;
}

static int LuaDumpStack(lua_State* L) {
    printf("--------------- LUA STACK DUMP ---------------\n");
    int top = lua_gettop(L);
    for (int i = top; i; --i) {
        printf("%d%s: ", i, i == top ? " (top)" : "");
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

static EcsEntity luaCreateEntity(lua_State *L, EcsType type) {
    luacs_newobject(L, "EcsEntity", NULL);
    LuaEntity *e = luacs_object_pointer(L, -1, "EcsEntity");
    e->id = EcsNewEntity(type);
    return e->id;
}

static int luaEcsNewEntity(lua_State *L) {
    luaCreateEntity(L, EcsNormal);
    return 1;
}

static int ComponentMemberCompare(const void *a, const void *b, void *udata) {
    return strcmp(((LuaComponentMember*)a)->name, ((LuaComponentMember*)b)->name);
}

static uint64_t ComponentMemberHash(const void *item, uint64_t seed0, uint64_t seed1) {
    LuaComponentMember *component = (LuaComponentMember*)item;
    return hashmap_sip(component->name, strlen(component->name), seed0, seed1);
}

static void ComponentMemberFree(void *item) {
    LuaComponentMember *member = (LuaComponentMember*)item;
    free((void*)member->name);
    if (member->value.type == LUA_TSTRING)
        free((void*)member->value.data.string);
}

#define LUA_TINTEGER 9

static int luaEcsNewComponent(lua_State *L) {
    const char *name = luaL_checkstring(L, -2);
    LuaComponent *search = malloc(sizeof(LuaComponent));
    search->name = name;
    LuaComponent *found = hashmap_get(world.components, (void*)search);
    if (found)
        luaL_error(L, "Component already named `%s`", name);
    search->map = hashmap_new(sizeof(LuaComponentMember), 0, 0, 0, ComponentMemberHash, ComponentMemberCompare, ComponentMemberFree, NULL);
    assert(search->map);
    
    assert(lua_istable(L, -1));
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        const char *key = luaL_checkstring(L, -2);
        LuaComponentMember *searchMember = malloc(sizeof(LuaComponentMember));
        searchMember->name = strdup(key);
        LuaComponentMember *foundMember = hashmap_get(search->map, (void*)searchMember);
        if (foundMember)
            luaL_error(L, "Component already has member named `%s`", key);
        switch (searchMember->value.type = lua_type(L, -1)) {
            case LUA_TSTRING:
                searchMember->value.data.string = strdup(luaL_checkstring(L, -1));
                break;
            case LUA_TBOOLEAN:
            case LUA_TNUMBER:
                if (lua_isnumber(L, -1))
                    searchMember->value.data.number = lua_tonumber(L, -1);
                else if (lua_isinteger(L, -1)) {
                    searchMember->value.type = LUA_TINTEGER;
                    searchMember->value.data.integer = lua_tointeger(L, -1);
                } else if (lua_isboolean(L, -1))
                    searchMember->value.data.integer = lua_toboolean(L, -1);
                else
                    luaL_error(L, "Unexpected type `%s`", lua_typename(L, searchMember->value.type));
                break;
            default:
                luaL_error(L, "Invalid type `%s`", lua_typename(L, searchMember->value.type));
        }
        hashmap_set(search->map, (void*)searchMember);
        lua_pop(L, 1);
    }
    EcsEntity e = luaCreateEntity(L, EcsComponent);
    EcsAssure(e, sizeof(LuaComponent));
    search->name = strdup(name);
    search->id.id = e.id;
    hashmap_set(world.components, (void*)search);
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

static int LuaEntityRealID(lua_State *L) {
    LuaEntity *self = luacs_object_pointer(L, -1, NULL);
    lua_pushinteger(L, self->id.parts.id);
    return 1;
}

static LuaComponent* LuaFindComponent(lua_State *L, int idx) {
    int type = lua_type(L, idx);
    switch (type) {
        case LUA_TSTRING: {
            LuaComponent search = {.name = luaL_checkstring(L, idx)};
            LuaComponent *found = hashmap_get(world.components, (void*)&search);
            if (!found)
                luaL_error(L, "Invalid component named `%s`", search.name);
            return found;
        }
        case LUA_TUSERDATA: {
            LuaEntity *e = (LuaEntity*)luacs_object_pointer(L, idx, "EcsEntity");
            if (!(EcsIsEntityValid(e->id)) || e->id.parts.flag != EcsComponent)
                luaL_error(L, "Invalid component");
            size_t iter = 0;
            void *item;
            while (hashmap_iter(world.components, &iter, &item)) {
                LuaComponent *component = (LuaComponent*)item;
                if (component->id.parts.id == e->id.parts.id)
                    return component;
            }
            break;
        }
        default:
            luaL_error(L, "Unexpected type `%s`", lua_typename(L, type));
    }
    return NULL;
}

static int LuaEntityAddComponent(lua_State *L) {
    LuaComponent *component = LuaFindComponent(L, -1);
    assert(component->id.id != EcsNil);
    LuaEntity *self = luacs_object_pointer(L, -2, NULL);
    ECS_ASSERT(EcsIsEntityValid(self->id), Entity, self->id);
    ECS_ASSERT(EcsIsEntityValid(component->id), Entity, component->id);
    EcsStorage *storage = EcsFind(component->id);
    assert(storage && !StorageHas(storage, self->id));
    
    LuaComponent *newComponent = (LuaComponent*)StorageEmplace(storage, self->id);
    newComponent->map = hashmap_new(sizeof(LuaComponentMember), 0, 0, 0, ComponentMemberHash, ComponentMemberCompare, ComponentMemberFree, NULL);
    newComponent->name = strdup(component->name);
    
    size_t iter = 0;
    void *item;
    while (hashmap_iter(component->map, &iter, &item)) {
        LuaComponentMember *member = (LuaComponentMember*)item;
        LuaComponentMember *newMember = malloc(sizeof(LuaComponentMember));
        newMember->name = strdup(member->name);
        switch (newMember->value.type = member->value.type) {
            case LUA_TSTRING:
                newMember->value.data.string = strdup(member->value.data.string);
                break;
            case LUA_TNUMBER:
                newMember->value.data.number = member->value.data.number;
                break;
            case LUA_TINTEGER:
            case LUA_TBOOLEAN:
                newMember->value.data.integer = member->value.data.integer;
                break;
            default:
                luaL_error(L, "Unexpected type `%s`", lua_typename(L, member->value.type));
        }
        hashmap_set(newComponent->map, (void*)newMember);
    }
    return 0;
}

static int LuaEntityGetComponent(lua_State *L) {
    LuaComponent *component = LuaFindComponent(L, -1);
    assert(component->id.id != EcsNil);
    LuaEntity *self = luacs_object_pointer(L, -2, NULL);
    EcsStorage *storage = EcsFind(component->id);
    assert(storage && StorageHas(storage, self->id));

    LuaComponent *luaComponent = (LuaComponent*)StorageGet(storage, self->id);
    assert(luaComponent);
    
    size_t iter = 0;
    void *item;
    lua_newtable(L);
    while (hashmap_iter(luaComponent->map, &iter, &item)) {
        LuaComponentMember *member = (LuaComponentMember*)item;
        lua_pushstring(L, member->name);
        switch (member->value.type) {
            case LUA_TSTRING:
                lua_pushstring(L, member->value.data.string);
                break;
            case LUA_TBOOLEAN:
                lua_pushboolean(L, (int)member->value.data.integer);
                break;
            case LUA_TINTEGER:
                lua_pushinteger(L, member->value.data.integer);
                break;
            case LUA_TNUMBER:
                lua_pushnumber(L, member->value.data.number);
                break;
            default:
                luaL_error(L, "Unexpected type `%s`", lua_typename(L, member->value.type));
        }
        lua_settable(L, -3);
    }
    return 1;
}

static int LuaEntitySetComponent(lua_State *L) {
    LuaComponent *component = LuaFindComponent(L, -2);
    assert(component->id.id != EcsNil);
    LuaEntity *self = luacs_object_pointer(L, -3, NULL);
    EcsStorage *storage = EcsFind(component->id);
    assert(storage && StorageHas(storage, self->id));
    
    LuaComponent *luaComponent = (LuaComponent*)StorageGet(storage, self->id);
    assert(luaComponent);
    
    assert(lua_istable(L, -1));
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        const char *key = luaL_checkstring(L, -2);
        LuaComponentMember searchMember = {.name = key};
        LuaComponentMember *foundMember = hashmap_get(luaComponent->map, (void*)&searchMember);
        if (!foundMember)
            luaL_error(L, "Component `%s` has no member named `%s`", luaComponent->name, key);
        switch (foundMember->value.type = lua_type(L, -1)) {
            case LUA_TSTRING:
                if (foundMember->value.data.string)
                    free((void*)foundMember->value.data.string);
                foundMember->value.data.string = lua_tostring(L, -1);
                break;
            case LUA_TBOOLEAN:
                foundMember->value.data.integer = lua_toboolean(L, -1);
                break;
            case LUA_TNUMBER:
                if (lua_isinteger(L, -1)) {
                    foundMember->value.type = LUA_TINTEGER;
                    foundMember->value.data.integer = lua_tointeger(L, -1);
                } else
                    foundMember->value.data.number = lua_tonumber(L, -1);
                break;
            default:
                luaL_error(L, "Unexpected type `%s`", lua_typename(L, foundMember->value.type));
        }
        lua_pop(L, 1);
    }
    return 0;
}

void LuaLoadEcs(lua_State *L) {
    luaL_requiref(L, "Ecs", &luaopen_Ecs, 1);
    lua_pop(L, 1);
    
    luacs_newenum(L, EcsType);
    luacs_enum_declare_value(L, "Entity",   EcsNormal);
    luacs_enum_declare_value(L, "Component", EcsComponent);
    luacs_enum_declare_value(L, "System",  EcsSystem);
    lua_setglobal(L, "EcsType");
    
    luacs_newstruct(L, EcsEntity);
    luacs_int_field(L, EcsEntity, id, 0);
    luacs_declare_method(L, "rid", LuaEntityRealID);
    luacs_declare_method(L, "add", LuaEntityAddComponent);
    luacs_declare_method(L, "get", LuaEntityGetComponent);
    luacs_declare_method(L, "set", LuaEntitySetComponent);
    lua_pop(L, 1);
    
    lua_pushcfunction(L, LuaDumpTable);
    lua_setglobal(L, "LuaDumpTable");
    lua_pushcfunction(L, LuaDumpStack);
    lua_setglobal(L, "LuaDumpStack");
}
