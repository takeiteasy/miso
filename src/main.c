//
//  main.c
//  colony
//
//  Created by George Watson on 08/02/2023.
//

#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sokol_glue.h"
#include "sokol_args.h"
#include "sokol_time.h"

#include "maths.h"
#include "ecs.h"
#include "texture.h"
#include "input.h"
#include "chunk.h"
#include "random.h"
#include "queue.h"

#include <stdbool.h>
#include <float.h>
#include <sys/time.h>

#define CAMERA_SPEED 10.f
#define CAMERA_CHASE_SPEED .1f

//! TODO: Camera zooming
//! TODO: Loading screen
//! TODO: Loading and saving chunks
//! TODO: Loading and saving game state
//! TODO: Sprite animations

static Entity EcsPositionComponent = EcsNilEntity;
static Entity EcsTargetComponent = EcsNilEntity;

static Entity EcsCamera = EcsNilEntity;
static Entity EcsNPC = EcsNilEntity;

Entity EcsChunkComponent = EcsNilEntity;
Entity EcsTextureBatchComponent = EcsNilEntity;

static struct {
    World *world;
    Random rng;
    Chunk *chunks[MAX_CHUNKS];
    int chunksSize;
    Entity chunkSearchSystem;
    float delta;
    TextureManager textures;
    TextureBatch *chunkTiles;
    Queue chunkQueue;
    pthread_t chunkThread;
    
    sg_pass_action pass_action;
    sg_pipeline pipeline;
} state;

static void UpdateCamera(Query *query) {
    Position *cameraPosition = EcsGet(state.world, query->entity, EcsPositionComponent);
    Vec2 *cameraTarget = EcsGet(state.world, query->entity, EcsTargetComponent);
    
    Vec2i move = (Vec2i){0,0};
    if (IsKeyDown(SAPP_KEYCODE_UP))
        move.y = -1;
    if (IsKeyDown(SAPP_KEYCODE_DOWN))
        move.y =  1;
    if (IsKeyDown(SAPP_KEYCODE_LEFT))
        move.x = -1;
    if (IsKeyDown(SAPP_KEYCODE_RIGHT))
        move.x =  1;
    
    *cameraTarget = (Vec2) {
        cameraTarget->x + (CAMERA_SPEED * state.delta * move.x),
        cameraTarget->y + (CAMERA_SPEED * state.delta * move.y)
    };
    *cameraPosition = MoveTowards(*cameraPosition, *cameraTarget, CAMERA_CHASE_SPEED);
    
    if (move.x != 0 || move.y != 0)
        EcsEnableSystem(state.world, state.chunkSearchSystem);
    
    memset(state.chunks, 0, MAX_CHUNKS * sizeof(Chunk*));
    state.chunksSize = 0;
}

static void UpdateChunks(Query *query) {
    Chunk *chunk = ECS_FIELD(query, Chunk, 0);
    Entity camera = EcsEntityNamed(state.world, "Camera");
    Vec2 cameraPosition = *(Vec2*)EcsGet(state.world, camera, EcsPositionComponent);
    Vec2 cameraSize = {sapp_width(), sapp_height()};
    ChunkState chunkState = CalcChunkState(chunk->x, chunk->y, cameraPosition, cameraSize);
    switch (chunkState) {
        case CHUNK_FREE:
            DestroyChunk(chunk);
            DestroyEntity(state.world, query->entity);
            break;
        case CHUNK_VISIBLE:
        case CHUNK_RESERVED:
            assert(state.chunksSize < MAX_CHUNKS);
            state.chunks[state.chunksSize++] = chunk;
            break;
    }
}

static void RenderChunks(Query *query) {
    Chunk *chunk = ECS_FIELD(query, Chunk, 0);
    Entity camera = EcsEntityNamed(state.world, "Camera");
    Vec2 cameraPosition = *(Vec2*)EcsGet(state.world, camera, EcsPositionComponent);
    Vec2 cameraSize = {sapp_width(), sapp_height()};
    RenderChunk(chunk, cameraPosition, cameraSize, state.chunkTiles);
}

static int CalcTile(unsigned char height) {
    switch (MIN(255, height)) {
        case 26 ... 37:
            return 0;
        case 38 ... 70:
            return 64;
        case 71 ... 100:
            return 32;
        case 101 ... 113:
            return 96;
        default:
            return 0;
    }
}

static void NewChunk(int x, int y) {
    Entity entity = AddChunk(state.world, x, y);
    Chunk *chunk = EcsGet(state.world, entity, EcsChunkComponent);
    chunk->x = x;
    chunk->y = y;
    QueueAdd(&state.chunkQueue, (void*)chunk);
}

static void ChunkSearch(Query *query) {
    Position *cameraPosition = EcsGet(state.world, query->entity, EcsPositionComponent);
    Vec2i cameraChunk = CalcChunk(*cameraPosition);
    Vec2 cameraSize = {sapp_width(), sapp_height()};
    for (int x = cameraChunk.x - 1; x < cameraChunk.x + 2; x++)
        for (int y = cameraChunk.y - 1; y < cameraChunk.y + 2; y++) {
            if (CalcChunkState(x, y, *cameraPosition, cameraSize) == CHUNK_FREE)
                continue;
            bool alreadyExists = false;
            for (int i = 0; i < state.chunksSize; i++)
                if (state.chunks[i]->x == x && state.chunks[i]->y == y) {
                    alreadyExists = true;
                    break;
                }
            if (!alreadyExists)
                NewChunk(x, y);
        }
}

static void CommitBatches(Query *query) {
    TextureBatch *batch = ECS_FIELD(query, TextureBatch, 0);
    CommitTextureBatch(batch);
}

static void ThreadWait(pthread_cond_t *cond, pthread_mutex_t *mtx, int delayMs) {
    struct timeval now;
    gettimeofday(&now,NULL);
    struct timespec wait = {
        .tv_sec = now.tv_sec+5,
        .tv_nsec = (now.tv_usec+1000UL*delayMs)*1000UL
    };
    
    pthread_mutex_lock(mtx);
    pthread_cond_timedwait(cond, mtx, &wait);
    pthread_mutex_unlock(mtx);
}

static void* ChunkQueueThread(void *arg) {
    pthread_cond_t waitCond;
    pthread_cond_init(&waitCond, NULL);
    pthread_mutex_t waitMutex;
    pthread_mutex_init(&waitMutex, NULL);
    
    for (;;) {
        if (state.chunkQueue.count < 1 || !state.chunkQueue.root.next || !state.chunkQueue.tail) {
            ThreadWait(&waitCond, &waitMutex, 100);
            continue;
        }
        
        QueueBucket *bucket = QueuePop(&state.chunkQueue);
        Chunk *chunk = (Chunk*)bucket->data;
        
        unsigned char *heightmap = PerlinFBM(CHUNK_WIDTH, CHUNK_HEIGHT, chunk->x * CHUNK_WIDTH, chunk->y * CHUNK_HEIGHT, 200.f, 2.f, .5f, 16, true, &state.rng);
        
        for (int x = 0; x < CHUNK_WIDTH; x++)
            for (int y = 0; y < CHUNK_HEIGHT; y++) {
                int i = y * CHUNK_WIDTH + x;
                chunk->tiles[i] = CalcTile(heightmap[i]);
            }
        
        free(bucket);
        free(heightmap);
    }
    return NULL;
}

static void init(void) {
    sg_setup(&(sg_desc){.context=sapp_sgcontext()});
    stm_setup();
    
    state.pass_action = (sg_pass_action) {
        .colors[0] = { .action=SG_ACTION_CLEAR, .value={0.f, 0.f, 0.f, 1.f} }
    };
    
    state.pipeline = sg_make_pipeline(&(sg_pipeline_desc) {
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
        .shader = sg_make_shader(sprite_program_shader_desc(sg_query_backend())),
        .layout = {
            .buffers[0].stride = sizeof(Vertex),
            .attrs = {
                [ATTR_sprite_vs_position].format=SG_VERTEXFORMAT_FLOAT2,
                [ATTR_sprite_vs_texcoord].format=SG_VERTEXFORMAT_FLOAT2
            }
        },
        .colors[0] = {
            .blend = {
                .enabled = true,
                .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
                .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                .op_rgb = SG_BLENDOP_ADD,
                .src_factor_alpha = SG_BLENDFACTOR_ONE,
                .dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                .op_alpha = SG_BLENDOP_ADD
            }
        }
    });
    
    state.rng = NewRandom(0);
    state.world = EcsNewWorld();
    
    EcsPositionComponent = ECS_COMPONENT(state.world, Vec2);
    EcsTargetComponent = ECS_COMPONENT(state.world, Vec2);
    EcsNPC = ECS_TAG(state.world);
    EcsChunkComponent = ECS_COMPONENT(state.world, Chunk);
    EcsTextureBatchComponent = ECS_COMPONENT(state.world, TextureBatch);
    
    EcsCamera = ECS_TAG(state.world);
    Entity view = EcsNewEntity(state.world);
    EcsName(state.world, view, "Camera");
    EcsAttach(state.world, view, EcsPositionComponent);
    Position *viewPosition = EcsGet(state.world, view, EcsPositionComponent);
    *viewPosition = (Vec2){CHUNK_WIDTH/2,CHUNK_HEIGHT/2};
    EcsAttach(state.world, view, EcsTargetComponent);
    Vec2 *viewTarget = EcsGet(state.world, view, EcsTargetComponent);
    *viewTarget = *viewPosition;
    EcsAttach(state.world, view, EcsCamera);
    
    ECS_SYSTEM(state.world, UpdateCamera, EcsCamera);
    ECS_SYSTEM(state.world, UpdateChunks, EcsChunkComponent);
    ECS_SYSTEM(state.world, RenderChunks, EcsChunkComponent);
    state.chunkSearchSystem = ECS_SYSTEM(state.world, ChunkSearch, EcsCamera);
    EcsDisableSystem(state.world, state.chunkSearchSystem);
    ECS_SYSTEM(state.world, CommitBatches, EcsTextureBatchComponent);
    
    state.textures = NewTextureManager();
    TextureManagerAdd(&state.textures, "assets/tiles.png");
    
    Entity chunkTiles = EcsNewEntity(state.world);
    EcsAttach(state.world, chunkTiles, EcsTextureBatchComponent);
    TextureBatch *chunkBatch = EcsGet(state.world, chunkTiles, EcsTextureBatchComponent);
    *chunkBatch = NewTextureBatch(TextureManagerGet(&state.textures, "assets/tiles.png"), CHUNK_SIZE);
    state.chunkTiles = chunkBatch;
    
    state.chunkQueue = NewQueue();
    for (int x = -1; x < 1; x++)
        for (int y = -1; y < 1; y++)
            NewChunk(x, y);
    pthread_create(&state.chunkThread, NULL, ChunkQueueThread, NULL);
}

static void frame(void) {
    state.delta = (float)(sapp_frame_duration() * 60.0);
    
    sg_begin_default_pass(&state.pass_action, sapp_width(), sapp_height());
    sg_apply_pipeline(state.pipeline);
    
    EcsStep(state.world);
    EcsDisableSystem(state.world, state.chunkSearchSystem);
    
    sg_end_pass();
    sg_commit();
    
    ResetInputHandler();
}

static void DestroyBatches(Query *query) {
    TextureBatch *batch = ECS_FIELD(query, TextureBatch, 0);
    DestroyTextureBatch(batch);
}

static void cleanup(void) {
    pthread_kill(state.chunkThread, 0);
    DestroyQueue(&state.chunkQueue);
    ECS_QUERY(state.world, DestroyBatches, NULL, EcsTextureBatchComponent);
    DestroyTextureManager(&state.textures);
    DestroyWorld(&state.world);
    sg_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[]) {
    sargs_setup(&(sargs_desc) {.argv=argv, .argc=argc});
    return (sapp_desc) {
        .init_cb = init,
        .frame_cb = frame,
        .event_cb = InputHandler,
        .cleanup_cb = cleanup,
        .window_title = "colony",
        .width = 640,
        .height = 480,
        .icon.sokol_default = true
    };
}
