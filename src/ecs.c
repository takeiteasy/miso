/* ecs.c -- https://github.com/takeiteasy/secs
 
 The MIT License (MIT)

 Copyright (c) 2022 George Watson

 Permission is hereby granted, free of charge, to any person
 obtaining a copy of this software and associated documentation
 files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge,
 publish, distribute, sublicense, and/or sell copies of the Software,
 and to permit persons to whom the Software is furnished to do so,
 subject to the following conditions:

 The above copyright notice and this permission notice shall be
 included in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. */

#include "ecs.h"
#if defined(DEBUG)
#define ASSERT(X) \
    do {                                                                                       \
        if (!(X)) {                                                                            \
            fprintf(stderr, "ERROR! Assertion hit! %s:%s:%d\n", __FILE__, __func__, __LINE__); \
            assert(false);                                                                     \
        }                                                                                      \
    } while(0)
#define ECS_ASSERT(X, Y, V)                                                                    \
    do {                                                                                       \
        if (!(X)) {                                                                            \
            fprintf(stderr, "ERROR! Assertion hit! %s:%s:%d\n", __FILE__, __func__, __LINE__); \
            Dump##Y(V);                                                                        \
            assert(false);                                                                     \
        }                                                                                      \
    } while(0)
#else
#define ECS_ASSERT(X, _, __) assert(X)
#define ASSERT(X) assert(X)
#endif

#define ECS_COMPOSE_ENTITY(ID, VER, TAG) \
    (Entity)                             \
    {                                    \
        .parts = {                       \
            .id = ID,                    \
            .version = VER,              \
            .flag = TAG                  \
        }                                \
    }

#define SAFE_FREE(X)    \
    do                  \
    {                   \
        if ((X))        \
        {               \
            free((X));  \
            (X) = NULL; \
        }               \
    } while (0)

Entity EcsEntityName = EcsNilEntity;
Entity EcsSystem     = EcsNilEntity;
Entity EcsPrefab     = EcsNilEntity;
Entity EcsRelation   = EcsNilEntity;
Entity EcsChildOf    = EcsNilEntity;
Entity EcsTimer      = EcsNilEntity;

typedef struct {
    Entity *sparse;
    Entity *dense;
    size_t sizeOfSparse;
    size_t sizeOfDense;
} EcsSparse;

typedef struct {
    Entity componentId;
    void *data;
    size_t sizeOfData;
    size_t sizeOfComponent;
    EcsSparse *sparse;
} EcsStorage;

#if defined(DEBUG)
static void DumpEntity(Entity e) {
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

static bool SparseHas(EcsSparse *sparse, Entity e) {
    ASSERT(sparse);
    uint32_t id = ENTITY_ID(e);
    ASSERT(id != EcsNil);
    return (id < sparse->sizeOfSparse) && (ENTITY_ID(sparse->sparse[id]) != EcsNil);
}

static void SparseEmplace(EcsSparse *sparse, Entity e) {
    ASSERT(sparse);
    uint32_t id = ENTITY_ID(e);
    ASSERT(id != EcsNil);
    if (id >= sparse->sizeOfSparse) {
        const size_t newSize = id + 1;
        sparse->sparse = realloc(sparse->sparse, newSize * sizeof * sparse->sparse);
        for (size_t i = sparse->sizeOfSparse; i < newSize; i++)
            sparse->sparse[i] = EcsNilEntity;
        sparse->sizeOfSparse = newSize;
    }
    sparse->sparse[id] = (Entity) { .parts = { .id = (uint32_t)sparse->sizeOfDense } };
    sparse->dense = realloc(sparse->dense, (sparse->sizeOfDense + 1) * sizeof * sparse->dense);
    sparse->dense[sparse->sizeOfDense++] = e;
}

static size_t SparseRemove(EcsSparse *sparse, Entity e) {
    ASSERT(sparse);
    ECS_ASSERT(SparseHas(sparse, e), Sparse, sparse);
    
    const uint32_t id = ENTITY_ID(e);
    uint32_t pos = ENTITY_ID(sparse->sparse[id]);
    Entity other = sparse->dense[sparse->sizeOfDense-1];
    
    sparse->sparse[ENTITY_ID(other)] = (Entity) { .parts = { .id = pos } };
    sparse->dense[pos] = other;
    sparse->sparse[id] = EcsNilEntity;
    sparse->dense = realloc(sparse->dense, --sparse->sizeOfDense * sizeof * sparse->dense);
    
    return pos;
}

static size_t SparseAt(EcsSparse *sparse, Entity e) {
    ASSERT(sparse);
    uint32_t id = ENTITY_ID(e);
    ASSERT(id != EcsNil);
    return ENTITY_ID(sparse->sparse[id]);
}

static EcsStorage* NewStorage(Entity id, size_t sz) {
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

static void DestroyStorage(EcsStorage **storage) {
    EcsStorage *s = *storage;
    if (!s || !storage)
        return;
    SAFE_FREE(s->sparse->sparse);
    SAFE_FREE(s->sparse->dense);
    SAFE_FREE(s);
    *storage = NULL;
}

static bool StorageHas(EcsStorage *storage, Entity e) {
    ASSERT(storage);
    ASSERT(!ENTITY_IS_NIL(e));
    return SparseHas(storage->sparse, e);
}

static void* StorageEmplace(EcsStorage *storage, Entity e) {
    ASSERT(storage);
    storage->data = realloc(storage->data, (storage->sizeOfData + 1) * sizeof(char) * storage->sizeOfComponent);
    storage->sizeOfData++;
    void *result = &((char*)storage->data)[(storage->sizeOfData - 1) * sizeof(char) * storage->sizeOfComponent];
    SparseEmplace(storage->sparse, e);
    return result;
}

static void StorageRemove(EcsStorage *storage, Entity e) {
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

static void* StorageGet(EcsStorage *storage, Entity e) {
    ASSERT(storage);
    ASSERT(!ENTITY_IS_NIL(e));
    return StorageAt(storage, SparseAt(storage->sparse, e));
}

struct World {
    EcsStorage **storages;
    size_t sizeOfStorages;
    Entity *entities;
    size_t sizeOfEntities;
    uint32_t *recyclable;
    size_t sizeOfRecyclable;
    uint32_t nextAvailableId;
};

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#elif defined(__APPLE__) && defined(__MACH__)
#include <mach/mach_time.h>
#elif defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#else /* anything else, this will need more care for non-Linux platforms */
#ifdef ESP8266
// On the ESP8266, clock_gettime ignores the first argument and CLOCK_MONOTONIC isn't defined
#define CLOCK_MONOTONIC 0
#endif
#include <time.h>
#endif

static struct {
    bool initalized;
#if defined(_WIN32)
    uint32_t initialized;
    LARGE_INTEGER freq;
    LARGE_INTEGER start;
#elif defined(__APPLE__) && defined(__MACH__)
    uint32_t initialized;
    mach_timebase_info_data_t timebase;
    uint64_t start;
#elif defined(__EMSCRIPTEN__)
    uint32_t initialized;
    double start;
#else /* anything else, this will need more care for non-Linux platforms */
    uint32_t initialized;
    uint64_t start;
#endif
} Time = {
    .initalized = false
};

static void InitTimers(void) {
    memset(&Time, 0, sizeof(Time));
    Time.initalized = true;
#if defined(_WIN32)
    QueryPerformanceFrequency(&Time.freq);
    QueryPerformanceCounter(&Time.start);
#elif defined(__APPLE__) && defined(__MACH__)
    mach_timebase_info(&Time.timebase);
    Time.start = mach_absolute_time();
#elif defined(__EMSCRIPTEN__)
    Time.start = emscripten_get_now();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    Time.start = (uint64_t)ts.tv_sec*1000000000 + (uint64_t)ts.tv_nsec;
#endif
}

static int64_t int64MulDiv(int64_t value, int64_t numer, int64_t denom) {
    int64_t q = value / denom;
    int64_t r = value % denom;
    return q * numer + r * numer / denom;
}

static uint64_t now(void) {
    assert(Time.initalized);
    uint64_t result = 0L;
#if defined(_WIN32)
    LARGE_INTEGER qpc_t;
    QueryPerformanceCounter(&qpc_t);
    result = (uint64_t)int64MulDiv(qpc_t.QuadPart - Time.start.QuadPart, 1000000000, Time.freq.QuadPart);
#elif defined(__APPLE__) && defined(__MACH__)
    const uint64_t mach_now = mach_absolute_time() - Time.start;
    result = (uint64_t)int64MulDiv((int64_t)mach_now, (int64_t)Time.timebase.numer, (int64_t)Time.timebase.denom);
#elif defined(__EMSCRIPTEN__)
    double js_now = emscripten_get_now() - Time.start;
    result = (uint64_t) (js_now * 1000000.0);
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    result = ((uint64_t)ts.tv_sec*1000000000 + (uint64_t)ts.tv_nsec) - Time.start;
#endif
    return result;
}

static uint64_t diff(uint64_t new_ticks, uint64_t old_ticks) {
    return new_ticks > old_ticks ? new_ticks - old_ticks : 1;
}

static void UpdateTimer(Query *query) {
    Timer *timer = ECS_FIELD(query, Timer, 0);
    if (!timer->enabled)
        return;
    uint64_t time = now();
    if (diff(time, timer->start) / 1000000.0 > timer->interval) {
        timer->cb(timer->userdata);
        timer->start = now();
    }
}

typedef struct {
    uint64_t hash;
} Name;

World* EcsNewWorld(void) {
    if (!Time.initalized)
        InitTimers();
    
    World *result = malloc(sizeof(World));
    *result = (World){0};
    result->nextAvailableId = EcsNil;
    
    EcsSystem     = ECS_COMPONENT(result, System);
    EcsPrefab     = ECS_COMPONENT(result, Prefab);
    EcsRelation   = ECS_COMPONENT(result, Relation);
    EcsChildOf    = ECS_TAG(result);
    EcsTimer      = ECS_COMPONENT(result, Timer);
    EcsEntityName = ECS_COMPONENT(result, Name);
    ECS_SYSTEM(result, UpdateTimer, EcsTimer);
    
    return result;
}

#define DEL_TYPES \
    X(System, 0)  \
    X(Prefab, 1)

#define X(TYPE, N)                                                       \
    case N: {                                                            \
        for (int j = 0; j < storage->sparse->sizeOfDense; j++) {         \
            TYPE *type = StorageGet(storage, storage->sparse->dense[j]); \
            free(type->components);                                      \
        }                                                                \
        break;                                                           \
    }
void DestroyWorld(World **world) {
    World *w = *world;
    if (!w || !world)
        return;
    if (w->storages)
        for (int i = 0; i < w->sizeOfStorages; i++) {
            EcsStorage *storage = w->storages[i];
            switch (i) {
                DEL_TYPES
            }
            DestroyStorage(&storage);
        }
    SAFE_FREE(w->storages);
    SAFE_FREE(w->entities);
    SAFE_FREE(w->recyclable);
    *world = NULL;
}
#undef X

static EcsStorage* EcsFind(World *world, Entity e) {
    for (int i = 0; i < world->sizeOfStorages; i++)
        if (ENTITY_ID(world->storages[i]->componentId) == ENTITY_ID(e))
            return world->storages[i];
    return NULL;
}

//-----------------------------------------------------------------------------
// SipHash reference C implementation
//
// Copyright (c) 2012-2016 Jean-Philippe Aumasson
// <jeanphilippe.aumasson@gmail.com>
// Copyright (c) 2012-2014 Daniel J. Bernstein <djb@cr.yp.to>
//
// To the extent possible under law, the author(s) have dedicated all copyright
// and related and neighboring rights to this software to the public domain
// worldwide. This software is distributed without any warranty.
//
// You should have received a copy of the CC0 Public Domain Dedication along
// with this software. If not, see
// <http://creativecommons.org/publicdomain/zero/1.0/>.
//
// default: SipHash-2-4
//-----------------------------------------------------------------------------
static uint64_t SIP64(const uint8_t *in, const size_t inlen, uint64_t seed) {
#define U8TO64_LE(p)                                            \
    {(((uint64_t)((p)[0])) | ((uint64_t)((p)[1]) << 8) |        \
      ((uint64_t)((p)[2]) << 16) | ((uint64_t)((p)[3]) << 24) | \
      ((uint64_t)((p)[4]) << 32) | ((uint64_t)((p)[5]) << 40) | \
      ((uint64_t)((p)[6]) << 48) | ((uint64_t)((p)[7]) << 56))}
#define U64TO8_LE(p, v)                            \
    {                                              \
        U32TO8_LE((p), (uint32_t)((v)));           \
        U32TO8_LE((p) + 4, (uint32_t)((v) >> 32)); \
    }
#define U32TO8_LE(p, v)                \
    {                                  \
        (p)[0] = (uint8_t)((v));       \
        (p)[1] = (uint8_t)((v) >> 8);  \
        (p)[2] = (uint8_t)((v) >> 16); \
        (p)[3] = (uint8_t)((v) >> 24); \
    }
#define ROTL(x, b) (uint64_t)(((x) << (b)) | ((x) >> (64 - (b))))
#define SIPROUND           \
    {                      \
        v0 += v1;          \
        v1 = ROTL(v1, 13); \
        v1 ^= v0;          \
        v0 = ROTL(v0, 32); \
        v2 += v3;          \
        v3 = ROTL(v3, 16); \
        v3 ^= v2;          \
        v0 += v3;          \
        v3 = ROTL(v3, 21); \
        v3 ^= v0;          \
        v2 += v1;          \
        v1 = ROTL(v1, 17); \
        v1 ^= v2;          \
        v2 = ROTL(v2, 32); \
    }
    uint64_t k = U8TO64_LE((uint8_t *)&seed);
    uint64_t v3 = UINT64_C(0x7465646279746573) ^ k;
    uint64_t v2 = UINT64_C(0x6c7967656e657261) ^ k;
    uint64_t v1 = UINT64_C(0x646f72616e646f6d) ^ k;
    uint64_t v0 = UINT64_C(0x736f6d6570736575) ^ k;
    const uint8_t *end = in + inlen - (inlen % sizeof(uint64_t));
    for (; in != end; in += 8) {
        uint64_t m = U8TO64_LE(in);
        v3 ^= m;
        SIPROUND;
        SIPROUND;
        v0 ^= m;
    }
    const int left = inlen & 7;
    uint64_t b = ((uint64_t)inlen) << 56;
    switch (left) {
    case 7:
        b |= ((uint64_t)in[6]) << 48;
    case 6:
        b |= ((uint64_t)in[5]) << 40;
    case 5:
        b |= ((uint64_t)in[4]) << 32;
    case 4:
        b |= ((uint64_t)in[3]) << 24;
    case 3:
        b |= ((uint64_t)in[2]) << 16;
    case 2:
        b |= ((uint64_t)in[1]) << 8;
    case 1:
        b |= ((uint64_t)in[0]);
        break;
    case 0:
        break;
    }
    v3 ^= b;
    SIPROUND;
    SIPROUND;
    v0 ^= b;
    v2 ^= 0xff;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    b = v0 ^ v1 ^ v2 ^ v3;
    uint64_t out = 0;
    U64TO8_LE((uint8_t *)&out, b);
    return out;
}

static uint64_t Hash(const char *string) {
    return SIP64((const void*)string, (int)strlen(string), 0);
}

static bool CheckNameUnique(World *world, uint64_t hash) {
    EcsStorage *names = EcsFind(world, EcsEntityName);
    for (size_t i = 0; i < world->sizeOfEntities; i++) {
        Entity e = world->entities[i];
        if (!StorageHas(names, e))
            continue;
        Name *entityName = StorageGet(names, e);
        if (entityName->hash == hash)
            return false;
    }
    return true;
}

bool EcsName(World *world, Entity entity, const char *name) {
    ASSERT(world);
    ECS_ASSERT(EcsIsValid(world, entity), Entity, entity);
    ASSERT(!!name);
    
    Name *result = NULL;
    uint64_t nameHash = Hash(name);
    EcsStorage *names = EcsFind(world, EcsEntityName);
    if (!names || !names->data || !names->sizeOfData)
        goto NEW_NAME;
    for (size_t i = 0; i < world->sizeOfEntities; i++) {
        Entity e = world->entities[i];
        if (!StorageHas(names, e))
            continue;
        if (ENTITY_CMP(e, entity))
            if (CheckNameUnique(world, nameHash)) {
                result = StorageGet(names, e);
                goto SET_NAME;
            }
        Name *entityName = StorageGet(names, e);
        if (entityName->hash == nameHash)
            return false;
    }
    
NEW_NAME:
    EcsAttach(world, entity, EcsEntityName);
    result = EcsGet(world, entity, EcsEntityName);
SET_NAME:
    result->hash = nameHash;
    return true;
}

Entity EcsEntityNamed(World *world, const char *name) {
    uint64_t nameHash = Hash(name);
    EcsStorage *names = EcsFind(world, EcsEntityName);
    for (size_t i = 0; i < world->sizeOfEntities; i++) {
        Entity e = world->entities[i];
        if (!StorageHas(names, e))
            continue;
        Name *entityName = StorageGet(names, e);
        if (entityName->hash == nameHash)
            return e;
    }
    return EcsNilEntity;
}

bool EcsIsValid(World *world, Entity e) {
    ASSERT(world);
    uint32_t id = ENTITY_ID(e);
    return id < world->sizeOfEntities && ENTITY_CMP(world->entities[id], e);
}

static Entity EcsNewEntityType(World *world, uint8_t type) {
    ASSERT(world);
    if (world->sizeOfRecyclable) {
        uint32_t idx = world->recyclable[world->sizeOfRecyclable-1];
        Entity e = world->entities[idx];
        Entity new = ECS_COMPOSE_ENTITY(ENTITY_ID(e), ENTITY_VERSION(e), type);
        world->entities[idx] = new;
        world->recyclable = realloc(world->recyclable, --world->sizeOfRecyclable * sizeof(uint32_t));
        return new;
    } else {
        world->entities = realloc(world->entities, ++world->sizeOfEntities * sizeof(Entity));
        Entity e = ECS_COMPOSE_ENTITY((uint32_t)world->sizeOfEntities-1, 0, type);
        world->entities[world->sizeOfEntities-1] = e;
        return e;
    }
}

Entity EcsNewEntity(World *world) {
    return EcsNewEntityType(world, EcsEntityType);
}

static EcsStorage* EcsAssure(World *world, Entity componentId, size_t sizeOfComponent) {
    EcsStorage *found = EcsFind(world, componentId);
    if (found)
        return found;
    EcsStorage *new = NewStorage(componentId, sizeOfComponent);
    world->storages = realloc(world->storages, (world->sizeOfStorages + 1) * sizeof * world->storages);
    world->storages[world->sizeOfStorages++] = new;
    return new;
}

bool EcsHas(World *world, Entity entity, Entity component) {
    ASSERT(world);
    ECS_ASSERT(EcsIsValid(world, entity), Entity, entity);
    ECS_ASSERT(EcsIsValid(world, component), Entity, component);
    return StorageHas(EcsFind(world, component), entity);
}

Entity EcsNewComponent(World *world, size_t sizeOfComponent) {
    Entity e = EcsNewEntityType(world, EcsComponentType);
    return EcsAssure(world, e, sizeOfComponent) ? e : EcsNilEntity;
}

Entity EcsNewSystem(World *world, SystemCb fn, size_t sizeOfComponents, ...) {
    Entity e = EcsNewEntityType(world, EcsSystemType);
    EcsAttach(world, e, EcsSystem);
    System *c = EcsGet(world, e, EcsSystem);
    c->callback = fn;
    c->sizeOfComponents = sizeOfComponents;
    c->components = malloc(sizeof(Entity) * sizeOfComponents);
    c->enabled = true;
    
    va_list args;
    va_start(args, sizeOfComponents);
    for (int i = 0; i < sizeOfComponents; i++)
        c->components[i] = va_arg(args, Entity);
    va_end(args);
    return e;
}

Entity EcsNewPrefab(World *world, size_t sizeOfComponents, ...) {
    Entity e = EcsNewEntityType(world, EcsPrefabType);
    EcsAttach(world, e, EcsPrefab);
    Prefab *c = EcsGet(world, e, EcsPrefab);
    c->sizeOfComponents = sizeOfComponents;
    c->components = malloc(sizeof(Entity) * sizeOfComponents);
    
    va_list args;
    va_start(args, sizeOfComponents);
    for (int i = 0; i < sizeOfComponents; i++)
        c->components[i] = va_arg(args, Entity);
    va_end(args);
    return e;
}

Entity EcsNewTimer(World *world, int interval, bool enable, TimerCb cb, void *userdata) {
    Entity e = EcsNewEntityType(world, EcsTimerType);
    EcsAttach(world, e, EcsTimer);
    Timer *timer = EcsGet(world, e, EcsTimer);
    timer->start = now();
    timer->enabled = enable;
    timer->interval = interval > 1 ? interval : 1;
    timer->cb = cb;
    timer->userdata = userdata;
    return e;
}

void DestroyEntity(World *world, Entity e) {
    ASSERT(world);
    ECS_ASSERT(EcsIsValid(world, e), Entity, e);
    switch (e.parts.flag) {
#define X(TYPE, _)                                 \
        case Ecs##TYPE##Type: {                    \
            TYPE *s = EcsGet(world, e, Ecs##TYPE); \
            if (s && s->components)                \
                free(s->components);               \
            break;                                 \
        }
        DEL_TYPES
#undef X
    }
    for (size_t i = world->sizeOfStorages; i; --i)
        if (world->storages[i - 1] && SparseHas(world->storages[i - 1]->sparse, e))
            StorageRemove(world->storages[i - 1], e);
    uint32_t id = ENTITY_ID(e);
    world->entities[id] = ECS_COMPOSE_ENTITY(id, ENTITY_VERSION(e) + 1, 0);
    world->recyclable = realloc(world->recyclable, ++world->sizeOfRecyclable * sizeof(uint32_t));
    world->recyclable[world->sizeOfRecyclable-1] = id;
}

void EcsAttach(World *world, Entity entity, Entity component) {
    switch (component.parts.flag) {
        case EcsRelationType: // Use EcsRelation()
        case EcsSystemType: // NOTE: potentially could be used for some sort of event system
        case EcsTimerType:
            ASSERT(false);
        case EcsPrefabType: {
            Prefab *c = EcsGet(world, component, EcsPrefab);
            for (int i = 0; i < c->sizeOfComponents; i++) {
                if (ENTITY_IS_NIL(c->components[i]))
                    break;
                EcsAttach(world, entity, c->components[i]);
            }
            break;
        }
        case EcsComponentType:
        default: {
            ECS_ASSERT(EcsIsValid(world, entity), Entity, entity);
            ECS_ASSERT(EcsIsValid(world, component), Entity, component);
            EcsStorage *storage = EcsFind(world, component);
            ASSERT(storage);
            StorageEmplace(storage, entity);
            break;
        }
    }
}

void EcsAssociate(World *world, Entity entity, Entity object, Entity relation) {
    ASSERT(world);
    ECS_ASSERT(EcsIsValid(world, entity), Entity, entity);
    ECS_ASSERT(EcsIsValid(world, object), Entity, object);
    ECS_ASSERT(ENTITY_ISA(object, Component), Entity, object);
    ECS_ASSERT(EcsIsValid(world, relation), Entity, relation);
    ECS_ASSERT(ENTITY_ISA(relation, Entity), Entity, relation);
    EcsAttach(world, entity, EcsRelation);
    Relation *pair = EcsGet(world, entity, EcsRelation);
    pair->object = object;
    pair->relation = relation;
}

void EcsDetach(World *world, Entity entity, Entity component) {
    ASSERT(world);
    ECS_ASSERT(EcsIsValid(world, entity), Entity, entity);
    ECS_ASSERT(EcsIsValid(world, component), Entity, component);
    EcsStorage *storage = EcsFind(world, component);
    ASSERT(storage);
    ECS_ASSERT(StorageHas(storage, entity), Storage, storage);
    StorageRemove(storage, entity);
}

void EcsDisassociate(World *world, Entity entity) {
    ECS_ASSERT(EcsIsValid(world, entity), Entity, entity);
    ECS_ASSERT(EcsHas(world, entity, EcsRelation), Entity, entity);
    EcsDetach(world, entity, EcsRelation);
}

bool EcsHasRelation(World *world, Entity entity, Entity object) {
    ECS_ASSERT(EcsIsValid(world, entity), Entity, entity);
    ECS_ASSERT(EcsIsValid(world, object), Entity, object);
    EcsStorage *storage = EcsFind(world, EcsRelation);
    if (!storage)
        return false;
    Relation *relation = StorageGet(storage, entity);
    if (!relation)
        return false;
    return ENTITY_CMP(relation->object, object);
}

bool EcsRelated(World *world, Entity entity, Entity relation) {
    ECS_ASSERT(EcsIsValid(world, entity), Entity, entity);
    ECS_ASSERT(EcsIsValid(world, relation), Entity, relation);
    EcsStorage *storage = EcsFind(world, EcsRelation);
    if (!storage)
        return false;
    Relation *_relation = StorageGet(storage, entity);
    if (!_relation)
        return false;
    return ENTITY_CMP(_relation->relation, relation);
}

void* EcsGet(World *world, Entity entity, Entity component) {
    ASSERT(world);
    ECS_ASSERT(EcsIsValid(world, entity), Entity, entity);
    ECS_ASSERT(EcsIsValid(world, component), Entity, component);
    EcsStorage *storage = EcsFind(world, component);
    ASSERT(storage);
    return StorageHas(storage, entity) ? StorageGet(storage, entity) : NULL;
}

void EcsSet(World *world, Entity entity, Entity component, const void *data) {
    ASSERT(world);
    ECS_ASSERT(EcsIsValid(world, entity), Entity, entity);
    ECS_ASSERT(EcsIsValid(world, component), Entity, component);
    EcsStorage *storage = EcsFind(world, component);
    ASSERT(storage);
    
    void *componentData = StorageHas(storage, entity) ?
                                    StorageGet(storage, entity) :
                                    StorageEmplace(storage, entity);
    ASSERT(componentData);
    memcpy(componentData, data, storage->sizeOfComponent);
}

static void DestroyQuery(Query *query) {
    SAFE_FREE(query->componentIndex);
    SAFE_FREE(query->componentData);
}

void EcsRelations(World *world, Entity parent, Entity relation, void *userdata, SystemCb cb) {
    EcsStorage *pairs = EcsFind(world, EcsRelation);
    for (size_t i = 0; i < world->sizeOfEntities; i++) {
        Entity e = world->entities[i];
        if (!StorageHas(pairs, e))
            continue;
        Relation *pair = StorageGet(pairs, e);
        if (!ENTITY_CMP(pair->object, relation) || !ENTITY_CMP(pair->relation, parent))
            continue;
        Query query = {
            .entity = e,
            .componentData = malloc(sizeof(void*)),
            .componentIndex = malloc(sizeof(Entity)),
            .sizeOfComponentData = 1,
            .userdata = userdata
        };
        query.componentIndex[0] = relation;
        query.componentData[0] = (void*)pair;
        cb(&query);
        DestroyQuery(&query);
    }
}

void EcsEnableSystem(World *world, Entity system) {
    ECS_ASSERT(EcsIsValid(world, system), Entity, system);
    ECS_ASSERT(ENTITY_ISA(system, System), Entity, system);
    System *s = EcsGet(world, system, EcsSystem);
    s->enabled = true;
}

void EcsDisableSystem(World *world, Entity system) {
    ECS_ASSERT(EcsIsValid(world, system), Entity, system);
    ECS_ASSERT(ENTITY_ISA(system, System), Entity, system);
    System *s = EcsGet(world, system, EcsSystem);
    s->enabled = false;
}

void EcsEnableTimer(World *world, Entity timer) {
    ECS_ASSERT(EcsIsValid(world, timer), Entity, timer);
    ECS_ASSERT(ENTITY_ISA(timer, Timer), Entity, timer);
    Timer *t = EcsGet(world, timer, EcsTimer);
    t->enabled = true;
    t->start = now();
}

void EcsDisableTimer(World *world, Entity timer) {
    ECS_ASSERT(EcsIsValid(world, timer), Entity, timer);
    ECS_ASSERT(ENTITY_ISA(timer, Timer), Entity, timer);
    Timer *t = EcsGet(world, timer, EcsTimer);
    t->enabled = false;
}

void EcsForce(World *world, Entity e) {
    ECS_ASSERT(EcsIsValid(world, e), Entity, e);
    ECS_ASSERT(ENTITY_ISA(e, System), Entity, e);
    System *system = EcsGet(world, e, EcsSystem);
    EcsQuery(world, system->callback, NULL, system->components, system->sizeOfComponents);
}

void EcsStep(World *world) {
    EcsStorage *storage = world->storages[ENTITY_ID(EcsSystem)];
    for (int i = 0; i < storage->sparse->sizeOfDense; i++) {
        System *system = StorageGet(storage, storage->sparse->dense[i]);
        if (system->enabled)
            EcsQuery(world, system->callback, NULL, system->components, system->sizeOfComponents);
    }
}

void EcsQuery(World *world, SystemCb cb, void *userdata, Entity *components, size_t sizeOfComponents) {
    for (size_t e = 0; e < world->sizeOfEntities; e++) {
        bool hasComponents = true;
        Query query = {
            .componentData = NULL,
            .componentIndex = NULL,
            .sizeOfComponentData = 0,
            .entity = world->entities[e],
            .userdata = userdata
        };
        
        for (size_t i = 0; i < sizeOfComponents; i++) {
            EcsStorage *storage = EcsFind(world, components[i]);
            if (!StorageHas(storage, world->entities[e])) {
                hasComponents = false;
                break;
            }
            
            query.sizeOfComponentData++;
            query.componentData = realloc(query.componentData, query.sizeOfComponentData * sizeof(void*));
            query.componentIndex = realloc(query.componentIndex, query.sizeOfComponentData * sizeof(Entity));
            query.componentIndex[query.sizeOfComponentData-1] = components[i];
            query.componentData[query.sizeOfComponentData-1] = StorageGet(storage, world->entities[e]);
        }
        
        if (hasComponents)
            cb(&query);
        DestroyQuery(&query);
    }
}

void* EcsQueryField(Query *query, size_t index) {
    return index >= query->sizeOfComponentData || ENTITY_IS_NIL(query->componentIndex[index]) ? NULL : query->componentData[index];
}
