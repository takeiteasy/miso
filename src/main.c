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
#include "renderer.h"
#include "input.h"
#include "chunk.h"

#include <stdbool.h>

#define CAMERA_SPEED 10.f
#define CAMERA_CHASE_SPEED .1f

static Entity EcsPositionComponent = EcsNilEntity;
static Entity EcsTargetComponent = EcsNilEntity;

static Entity EcsCamera = EcsNilEntity;
static Entity EcsNPC = EcsNilEntity;

Entity EcsChunkComponent = EcsNilEntity;

static Entity EcsRenderPass = EcsNilEntity;

typedef enum {
    ChunkPass,
    TOTAL_PASSES
} Pass;

static struct {
    World *world;
    Chunk *chunks[MAX_CHUNKS];
    int chunksSize;
    Entity chunkSearchSystem;
    float delta;
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
    Entity camera = EcsEntityNamed(state.world, "View");
    Vec2 cameraPosition = *(Vec2*)EcsGet(state.world, camera, EcsPositionComponent);
    Vec2 cameraSize = {sapp_width(), sapp_height()};
    ChunkState chunkState = CalcChunkState(chunk->x, chunk->y, cameraPosition, cameraSize);
    switch (chunkState) {
        case CHUNK_FREE:
            DestroyEntity(state.world, query->entity);
            break;
        case CHUNK_VISIBLE:
        case CHUNK_RESERVED:
            assert(state.chunksSize < MAX_CHUNKS);
            state.chunks[state.chunksSize++] = chunk;
            break;
    }
}

static void ResetPasses(Query *query) {
    RenderPass *pass = ECS_FIELD(query, RenderPass, 0);
    RenderPassBegin(pass);
}

static void RenderChunks(Query *query) {
    Chunk *chunk = ECS_FIELD(query, Chunk, 0);
    Entity camera = EcsEntityNamed(state.world, "View");
    Vec2 cameraPosition = *(Vec2*)EcsGet(state.world, camera, EcsPositionComponent);
    Vec2 cameraSize = {sapp_width(), sapp_height()};
    Entity pass = EcsEntityNamed(state.world, "ChunkPass");
    RenderPass *renderPass = EcsGet(state.world, pass, EcsRenderPass);
    TextureBatch *batch = RenderPassGetBatch(renderPass, "assets/tiles.png");
    RenderChunk(chunk, cameraPosition, cameraSize, batch);
}

static void FlushPasses(Query *query) {
    RenderPass *pass = ECS_FIELD(query, RenderPass, 0);
    RenderPassEnd(pass);
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
                AddChunk(state.world, x, y);
        }
}

static Entity AddPass(int w, int h, const char *name) {
    Entity pass = EcsNewEntity(state.world);
    EcsAttach(state.world, pass, EcsRenderPass);
    RenderPass *renderPass = EcsGet(state.world, pass, EcsRenderPass);
    *renderPass = NewRenderPass(0, 0);
    EcsName(state.world, pass, name);
    return pass;
}

static void init(void) {
    sg_setup(&(sg_desc){.context=sapp_sgcontext()});
    stm_setup();
    
    state.world = EcsNewWorld();
    
    EcsPositionComponent = ECS_COMPONENT(state.world, Vec2);
    EcsTargetComponent = ECS_COMPONENT(state.world, Vec2);
    EcsNPC = ECS_TAG(state.world);
    EcsChunkComponent = ECS_COMPONENT(state.world, Chunk);
    
    EcsCamera = ECS_TAG(state.world);
    Entity view = EcsNewEntity(state.world);
    EcsName(state.world, view, "View");
    EcsAttach(state.world, view, EcsPositionComponent);
    Position *viewPosition = EcsGet(state.world, view, EcsPositionComponent);
    *viewPosition = (Vec2){0,0};
    EcsAttach(state.world, view, EcsTargetComponent);
    Vec2 *viewTarget = EcsGet(state.world, view, EcsTargetComponent);
    *viewTarget = *viewPosition;
    EcsAttach(state.world, view, EcsCamera);
    
    EcsRenderPass = ECS_COMPONENT(state.world, RenderPass);
    
    Entity chunkPass = AddPass(0, 0, "ChunkPass");
    RenderPass *chunkRenderPass = EcsGet(state.world, chunkPass, EcsRenderPass);
    RenderPassNewBatch(chunkRenderPass, "assets/tiles.png", CHUNK_SIZE);
    
    ECS_SYSTEM(state.world, UpdateCamera, EcsCamera);
    ECS_SYSTEM(state.world, UpdateChunks, EcsChunkComponent);
    ECS_SYSTEM(state.world, ResetPasses, EcsRenderPass);
    ECS_SYSTEM(state.world, RenderChunks, EcsChunkComponent);
    ECS_SYSTEM(state.world, FlushPasses, EcsRenderPass);
    state.chunkSearchSystem = ECS_SYSTEM(state.world, ChunkSearch, EcsCamera);
    EcsDisableSystem(state.world, state.chunkSearchSystem);
    
    for (int x = -1; x < 1; x++)
        for (int y = -1; y < 1; y++)
            AddChunk(state.world, x, y);
}

static void frame(void) {
    state.delta = (float)(sapp_frame_duration() * 60.0);
    EcsStep(state.world);
    EcsDisableSystem(state.world, state.chunkSearchSystem);
    sg_commit();
    ResetInputHandler();
}

static void cleanup(void) {
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
