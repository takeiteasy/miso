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

#include <stdbool.h>

static Entity EcsPositionComponent;
static Entity EcsCameraComponent;
static Entity EcsChunkComponent;

#define TILE_WIDTH 32
#define TILE_HEIGHT 16
#define HALF_TILE_WIDTH (TILE_WIDTH/2)
#define HALF_TILE_HEIGHT (TILE_HEIGHT/2)

#define CHUNK_WIDTH 512
#define CHUNK_HEIGHT CHUNK_WIDTH
#define CHUNK_SIZE (CHUNK_WIDTH * CHUNK_HEIGHT)
#define CHUNK_AT(X, Y) ((Y) * CHUNK_WIDTH + (X))

#define CHUNK_REAL_WIDTH (CHUNK_WIDTH * TILE_WIDTH)
#define CHUNK_REAL_HEIGHT (CHUNK_HEIGHT * HALF_TILE_HEIGHT)

#define MAX_CHUNKS 8

typedef enum {
    CHUNK_FREE,
    CHUNK_RESERVED,
    CHUNK_VISIBLE
} ChunkState;

typedef struct {
    int x, y;
} Chunk;

typedef struct {
    Vec2 position;
    Vec2 target;
} Camera;

static struct {
    World *world;
    Chunk *chunks[MAX_CHUNKS];
    int chunksSize;
    RenderPass chunkPass;
    Vec2 screen;
    float delta;
} state;

static struct {
    bool button_down[SAPP_MAX_KEYCODES];
    bool button_clicked[SAPP_MAX_KEYCODES];
    bool mouse_down[SAPP_MAX_MOUSEBUTTONS];
    bool mouse_clicked[SAPP_MAX_MOUSEBUTTONS];
    Vec2 mouse_pos, last_mouse_pos;
    Vec2 mouse_scroll_delta, mouse_delta;
} Input;

static bool IsKeyDown(sapp_keycode key) {
    return Input.button_down[key];
}

static bool IsKeyUp(sapp_keycode key) {
    return !Input.button_down[key];
}

static bool WasKeyClicked(sapp_keycode key) {
    return Input.button_clicked[key];
}

static bool IsButtonDown(sapp_mousebutton button) {
    return Input.mouse_down[button];
}

static bool IsButtonUp(sapp_mousebutton button) {
    return !Input.mouse_down[button];
}

static bool WasButtonPressed(sapp_mousebutton button) {
    return Input.mouse_clicked[button];
}

static bool DoRectsCollide(Rect a, Rect b) {
    return a.pos.x < b.pos.x + b.size.x &&
           a.pos.x + a.size.x > b.pos.x &&
           a.pos.y < b.pos.y + b.size.y &&
           a.size.y + a.pos.y > b.pos.y;
}

static ChunkState CalcChunkState(int x, int y, Vec2 cameraPosition, Vec2 cameraSize) {
    Rect cameraRect = {
        .pos = cameraPosition - (cameraSize / 2),
        .size = cameraSize
    };
    Rect reservedRect = {
        .pos = cameraPosition - cameraSize,
        .size = cameraSize * 2.f
    };
    static Vec2 chunkSize = {
        CHUNK_REAL_WIDTH,
        CHUNK_REAL_HEIGHT
    };
    Rect chunkRect = {
        .pos = {
            x * chunkSize.x,
            y * chunkSize.y
        },
        .size = chunkSize
    };
    return DoRectsCollide(cameraRect, chunkRect) ? CHUNK_VISIBLE : DoRectsCollide(reservedRect, chunkRect) ? CHUNK_RESERVED : CHUNK_FREE;
}

static int IntToIndex(int i) {
    return abs(i * 2) - (i > 0 ? 1 : 0);
}

static int ChunkIndex(int x, int y) {
    int _x = IntToIndex(x), _y = IntToIndex(y);
    return _x >= _y ? _x * _x + _x + _y : _x + _y * _y;
}

static void RenderChunk(Query *query) {
    Chunk *chunk = ECS_FIELD(query, Chunk, 0);
    Vec2 cameraPosition = *(Vec2*)query->userdata;
    ChunkState chunkState = CalcChunkState(chunk->x, chunk->y, cameraPosition, state.screen);
    TextureBatch *batch = RenderPassGetBatch(&state.chunkPass, "assets/tiles.png");
    switch (chunkState) {
        case CHUNK_FREE:
            printf("DELETE CHUNK %d, %d\n", chunk->x, chunk->y);
            DestroyEntity(state.world, query->entity);
            break;
        case CHUNK_VISIBLE:;
            Vec2 chunkPosition = (Vec2){chunk->x * CHUNK_WIDTH, chunk->y * CHUNK_HEIGHT};
            Vec2 offset = chunkPosition * (Vec2){TILE_WIDTH,HALF_TILE_HEIGHT} + (-cameraPosition + state.screen / 2);
            Rect viewportBounds = {{0, 0}, state.screen};
            
            for (int x = 0; x < CHUNK_WIDTH; x++)
                for (int y = 0; y < CHUNK_HEIGHT; y++) {
                    float px = offset.x + ((float)x * (float)TILE_WIDTH) + (y % 2 ? HALF_TILE_WIDTH : 0);
                    float py = offset.y + ((float)y * (float)TILE_HEIGHT) - (y * HALF_TILE_HEIGHT);
                    Rect bounds = {{px, py}, {TILE_WIDTH, TILE_HEIGHT}};
                    if (!DoRectsCollide(viewportBounds, bounds))
                        continue;
                    TextureBatchRender(batch, (Vec2){px,py}, (Vec2){TILE_WIDTH,TILE_HEIGHT}, (Vec2){1.f,1.f}, 0.f, (Rect){{ChunkIndex(chunk->x, chunk->y) % 4 * 32, 0}, {TILE_WIDTH, TILE_HEIGHT}});
                }
        case CHUNK_RESERVED:
            state.chunks[state.chunksSize++] = chunk;
            break;
    }
}

#define CAMERA_SPEED 10.f
#define EPSILON .000001f
#define CAMERA_CHASE_SPEED .3f

static float Vec2Dist(Vec2 a, Vec2 b) {
    return sqrtf((a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y));
}

static void AddChunk(int x, int y) {
    Entity e = EcsNewEntity(state.world);
    EcsAttach(state.world, e, EcsChunkComponent);
    Chunk *chunk = EcsGet(state.world, e, EcsChunkComponent);
    chunk->x = x;
    chunk->y = y;
}

Vec2i CalcChunk(int x, int y) {
    return (Vec2i) {
        (int)floorf(x / (float)CHUNK_REAL_WIDTH),
        (int)floorf(y / (float)CHUNK_REAL_HEIGHT)
    };
}

static void ChunkPass(RenderPass *pass) {
    EcsStep(state.world);
}

static void UpdateCamera(Query *query) {
    Camera *camera = ECS_FIELD(query, Camera, 0);
    
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
        camera->target = (Vec2) {
            camera->target.x + (CAMERA_SPEED * state.delta * move.x),
            camera->target.y + (CAMERA_SPEED * state.delta * move.y)
        };
    
    float dist = Vec2Dist(camera->position, camera->target);
    if (dist > EPSILON) {
        Vec2 dv = (camera->target - camera->position) / dist;
        float min_step = MAX(0, dist - 100.f);
        camera->position = camera->position + dv * (min_step + ((dist - EPSILON) - min_step) * CAMERA_CHASE_SPEED);
    }
    
    memset(state.chunks, 0, MAX_CHUNKS * sizeof(Chunk*));
    state.chunksSize = 0;
    ECS_QUERY(state.world, RenderChunk, (void*)&camera->position, EcsChunkComponent);
    
    if (moved) {
        Vec2i cameraChunk = CalcChunk(camera->position.x, camera->position.y);
        for (int x = cameraChunk.x - 1; x < cameraChunk.x + 2; x++)
            for (int y = cameraChunk.y - 1; y < cameraChunk.y + 2; y++) {
                if (CalcChunkState(x, y, camera->position, state.screen) == CHUNK_FREE)
                    continue;
                bool alreadyExists = false;
                for (int i = 0; i < state.chunksSize; i++)
                    if (state.chunks[i]->x == x && state.chunks[i]->y == y) {
                        alreadyExists = true;
                        break;
                    }
                if (!alreadyExists)
                    AddChunk(x, y);
            }
    }
}

static void init(void) {
    sg_setup(&(sg_desc){
        .context = sapp_sgcontext()
    });
    stm_setup();
    
    state.world = EcsNewWorld();
    state.screen = (Vec2){sapp_width(), sapp_height()};
    
    state.chunkPass = NewRenderPass(state.screen.x, state.screen.y, ChunkPass);
    RenderPassNewBatch(&state.chunkPass, "assets/tiles.png", CHUNK_SIZE);
 
    EcsPositionComponent = ECS_COMPONENT(state.world, Vec2);
    EcsChunkComponent = ECS_COMPONENT(state.world, Chunk);
    EcsCameraComponent = ECS_COMPONENT(state.world, Camera);
    
    Entity mainCamera = EcsNewEntity(state.world);
    EcsAttach(state.world, mainCamera, EcsCameraComponent);
    Camera *camera = EcsGet(state.world, mainCamera, EcsCameraComponent);
    camera->position = camera->target = (Vec2){0,0};
    
    for (int x = -1; x < 1; x++)
        for (int y = -1; y < 1; y++)
            AddChunk(x, y);
    
    ECS_SYSTEM(state.world, UpdateCamera, EcsCameraComponent);
}

static void frame(void) {
    state.delta = (float)(sapp_frame_duration() * 60.0);
    
    RunRenderPass(&state.chunkPass);
    
    sg_commit();
    
    Input.mouse_delta = Input.mouse_scroll_delta = (Vec2){0};
    for (int i = 0; i < SAPP_MAX_KEYCODES; i++)
        if (Input.button_clicked[i])
            Input.button_clicked[i] = false;
    for (int i = 0; i < SAPP_MAX_MOUSEBUTTONS; i++)
        if (Input.mouse_clicked[i])
            Input.mouse_clicked[i] = false;
}

static void input(const sapp_event *e) {
    switch (e->type) {
        case SAPP_EVENTTYPE_KEY_DOWN:
#if defined(DEBUG)
            if (e->modifiers & SAPP_MODIFIER_SUPER && e->key_code == SAPP_KEYCODE_W)
                sapp_quit();
#endif
            Input.button_down[e->key_code] = true;
            break;
        case SAPP_EVENTTYPE_KEY_UP:
            Input.button_down[e->key_code] = false;
            Input.button_clicked[e->key_code] = true;
            break;
        case SAPP_EVENTTYPE_MOUSE_DOWN:
            Input.mouse_down[e->mouse_button] = true;
            break;
        case SAPP_EVENTTYPE_MOUSE_UP:
            Input.mouse_down[e->mouse_button] = false;
            Input.mouse_clicked[e->mouse_button] = true;
            break;
        case SAPP_EVENTTYPE_MOUSE_MOVE:
            Input.last_mouse_pos = Input.mouse_pos;
            Input.mouse_pos = (Vec2){e->mouse_x, e->mouse_y};
            Input.mouse_delta = (Vec2){e->mouse_dx, e->mouse_dy};
            break;
        case SAPP_EVENTTYPE_MOUSE_SCROLL:
            Input.mouse_scroll_delta = (Vec2){e->scroll_x, e->scroll_y};
            break;
        case SAPP_EVENTTYPE_RESIZED:
            state.screen = (Vec2){e->window_width, e->window_height};
            break;
        default:
            break;
    }
}

static void cleanup(void) {
    DestroyRenderPass(&state.chunkPass);
    DestroyWorld(&state.world);
    sg_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[]) {
    sargs_setup(&(sargs_desc) { .argv = argv, .argc = argc });
    return (sapp_desc) {
        .init_cb = init,
        .frame_cb = frame,
        .event_cb = input,
        .cleanup_cb = cleanup,
        .window_title = "colony",
        .width = 640,
        .height = 480,
        .icon.sokol_default = true
    };
}
