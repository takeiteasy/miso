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

#include "linalgb.h"
#include "ecs.h"
#include "renderer.h"
#include "input.h"
#include "chunk.h"

#include <stdbool.h>

#define CAMERA_SPEED 10.f
#define EPSILON .000001f
#define CAMERA_CHASE_SPEED .3f

static Entity EcsPositionComponent = EcsNilEntity;
Entity EcsChunkComponent = EcsNilEntity;

typedef struct {
    Vec2 position;
    Vec2 target;
} Camera;

static struct {
    World *world;
    Chunk *chunks[MAX_CHUNKS];
    Camera camera;
    int chunksSize;
    RenderPass chunkPass;
    float delta;
} state;

static void UpdateChunks(Query *query) {
    Chunk *chunk = ECS_FIELD(query, Chunk, 0);
    Vec2 cameraPosition = *(Vec2*)query->userdata;
    Vec2 cameraSize = {sapp_width(), sapp_height()};
    ChunkState chunkState = CalcChunkState(chunk->x, chunk->y, cameraPosition, cameraSize);
    switch (chunkState) {
        case CHUNK_FREE:
            DestroyEntity(state.world, query->entity);
            break;
        case CHUNK_VISIBLE:
        case CHUNK_RESERVED:
            state.chunks[state.chunksSize++] = chunk;
            break;
    }
}

static void RenderChunks(Query *query) {
    Chunk *chunk = ECS_FIELD(query, Chunk, 0);
    Vec2 cameraPosition = *(Vec2*)query->userdata;
    Vec2 cameraSize = {sapp_width(), sapp_height()};
    TextureBatch *batch = RenderPassGetBatch(&state.chunkPass, "assets/tiles.png");
    RenderChunk(state.world, chunk, cameraPosition, cameraSize, batch);
}

static void init(void) {
    sg_setup(&(sg_desc){
        .context = sapp_sgcontext()
    });
    stm_setup();
    
    state.chunkPass = NewRenderPass(0, 0);
    RenderPassNewBatch(&state.chunkPass, "assets/tiles.png", CHUNK_SIZE);
 
    state.world = EcsNewWorld();
    EcsPositionComponent = ECS_COMPONENT(state.world, Vec2);
    EcsChunkComponent = ECS_COMPONENT(state.world, Chunk);
    
    state.camera.position = state.camera.target = (Vec2){0,0};
    
    for (int x = -1; x < 1; x++)
        for (int y = -1; y < 1; y++)
            AddChunk(state.world, x, y);
}

static void frame(void) {
    state.delta = (float)(sapp_frame_duration() * 60.0);
    Vec2i move = (Vec2i){0,0};
    if (IsKeyDown(SAPP_KEYCODE_UP))
        move.y = -1;
    if (IsKeyDown(SAPP_KEYCODE_DOWN))
        move.y =  1;
    if (IsKeyDown(SAPP_KEYCODE_LEFT))
        move.x = -1;
    if (IsKeyDown(SAPP_KEYCODE_RIGHT))
        move.x =  1;
    
    bool moved = move.x != 0 || move.y != 0;
    if (moved)
        state.camera.target = (Vec2) {
            state.camera.target.x + (CAMERA_SPEED * state.delta * move.x),
            state.camera.target.y + (CAMERA_SPEED * state.delta * move.y)
        };
    
    float dist = Vec2Dist(state.camera.position, state.camera.target);
    if (dist > EPSILON) {
        Vec2 dv = (state.camera.target - state.camera.position) / dist;
        float min_step = MAX(0, dist - 100.f);
        state.camera.position = state.camera.position + dv * (min_step + ((dist - EPSILON) - min_step) * CAMERA_CHASE_SPEED);
    }
    
    memset(state.chunks, 0, MAX_CHUNKS * sizeof(Chunk*));
    state.chunksSize = 0;
    ECS_QUERY(state.world, UpdateChunks, (void*)&state.camera.position, EcsChunkComponent);
    
    RenderPassBegin(&state.chunkPass);
    ECS_QUERY(state.world, RenderChunks, (void*)&state.camera.position, EcsChunkComponent);
    RenderPassEnd(&state.chunkPass);
    
    if (moved) {
        Vec2i cameraChunk = CalcChunk(state.camera.position.x, state.camera.position.y);
        Vec2 cameraSize = {sapp_width(), sapp_height()};
        for (int x = cameraChunk.x - 1; x < cameraChunk.x + 2; x++)
            for (int y = cameraChunk.y - 1; y < cameraChunk.y + 2; y++) {
                if (CalcChunkState(x, y, state.camera.position, cameraSize) == CHUNK_FREE)
                    continue;
                bool alreadyExists = false;
                for (int i = 0; i < state.chunksSize; i++)
                    if (state.chunks[i]->x == x && state.chunks[i]->y == y) {
                        alreadyExists = true;
                        break;
                    }
                if (!alreadyExists)
                    AddChunk(state.world, x, y);
            }
    }
    
    sg_commit();
    ResetInputHandler();
}

static void cleanup(void) {
    DestroyRenderPass(&state.chunkPass);
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
