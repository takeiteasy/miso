//
//  main.c
//  colony
//
//  Created by George Watson on 08/02/2023.
//

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_STANDARD_VARARGS
#include "nuklear.h"

#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sokol_glue.h"
#include "sokol_args.h"
#include "sokol_time.h"
#include "sokol_nuklear.h"

#include "maths.h"
#include "ecs.h"
#include "texture.h"
#include "input.h"
#include "random.h"
#include "log.h"

#include <sys/time.h>

#define CAMERA_SPEED 10.f
#define CAMERA_CHASE_SPEED .1f

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

//! TODO: Minimap
//! TODO: Cursor position to world/grid position
//! TODO: Lua integration
//! TODO: Clean up input handling
//! TODO: Loading screen
//! TODO: Loading and saving game state
//! TODO: Sprite animations
//! TODO: Config file parsing json or ini

static Entity EcsPositionComponent = EcsNilEntity;
static Entity EcsTargetComponent = EcsNilEntity;

static Entity EcsCamera = EcsNilEntity;
static Entity EcsNPC = EcsNilEntity;

Entity EcsTextureBatchComponent = EcsNilEntity;

typedef struct {
    unsigned char *heightmap;
    int tiles[CHUNK_SIZE];
} Map;

static struct {
    World *world;
    Random rng;
    float delta;
    float zoom;
    Vec2 mouseDownPos;
    
    Map map;
    TextureManager textures;
    TextureBatch chunkTiles;
    
    sg_pass_action pass_action;
    sg_pipeline pipeline;
} state;

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

static void InitMap(void) {
    state.map.heightmap = PerlinFBM(CHUNK_WIDTH, CHUNK_HEIGHT, 0.f, 0.f, 0.f, 200.f, 2.f, .5f, 8);
    for (int x = 0; x < CHUNK_WIDTH; x++)
        for (int y = 0; y < CHUNK_HEIGHT; y++)
            state.map.tiles[CHUNK_AT(x, y)] = CalcTile(state.map.heightmap[CHUNK_AT(x, y)]);
}

static void RenderMap(Vec2 cameraPosition, Vec2 cameraSize, float cameraScale) {
    static Vec2 tile = (Vec2){TILE_WIDTH,TILE_HEIGHT};
    Vec2 offset = (tile/2) + (-cameraPosition + cameraSize / 2);
    for (int x = 0; x < CHUNK_WIDTH; x++)
        for (int y = 0; y < CHUNK_HEIGHT; y++) {
            Vec2 p = (Vec2) {
                offset.x + ((float)x * (float)TILE_WIDTH) + (y % 2 ? HALF_TILE_WIDTH : 0),
                offset.y + ((float)y * (float)TILE_HEIGHT) - (y * HALF_TILE_HEIGHT)
            } - (tile / 2);
            TextureBatchRender(&state.chunkTiles, p, tile, (Vec2){cameraScale,cameraScale}, cameraSize, 0.f, (Rect){{state.map.tiles[CHUNK_AT(x, y)], 0}, tile});
        }
}

static void UpdateCameraTarget(Query *query) {
    Position *cameraPosition = EcsGet(state.world, query->entity, EcsPositionComponent);
    Vec2 *cameraTarget = EcsGet(state.world, query->entity, EcsTargetComponent);
    *cameraTarget = *cameraPosition;
}

static void UpdateCamera(Query *query) {
    Position *cameraPosition = EcsGet(state.world, query->entity, EcsPositionComponent);
    Vec2 *cameraTarget = EcsGet(state.world, query->entity, EcsTargetComponent);
    
    if (WasMouseScrolled())
        state.zoom = CLAMP(state.zoom + MouseScroll().y, .1f, 2.f);
    
    if (IsButtonDown(SAPP_MOUSEBUTTON_LEFT)) {
        Vec2 delta = MousePosition() - state.mouseDownPos;
        Vec2 oldPos = *cameraTarget;
        *cameraPosition = oldPos - delta;
        *cameraTarget = oldPos;
    } else {
        Vec2 move = (Vec2){0,0};
        if (IsKeyDown(SAPP_KEYCODE_UP) || IsKeyDown(SAPP_KEYCODE_W))
            move.y = -1;
        if (IsKeyDown(SAPP_KEYCODE_DOWN) || IsKeyDown(SAPP_KEYCODE_S))
            move.y =  1;
        if (IsKeyDown(SAPP_KEYCODE_LEFT) || IsKeyDown(SAPP_KEYCODE_A))
            move.x = -1;
        if (IsKeyDown(SAPP_KEYCODE_RIGHT) || IsKeyDown(SAPP_KEYCODE_D))
            move.x =  1;
        
        *cameraTarget = *cameraTarget + (CAMERA_SPEED * state.delta * move);
        *cameraPosition = MoveTowards(*cameraPosition, *cameraTarget, CAMERA_CHASE_SPEED);
    }
    
    RenderMap(*cameraPosition, (Vec2){sapp_width(), sapp_height()}, state.zoom);
}


static void init(void) {
    sg_setup(&(sg_desc){.context=sapp_sgcontext()});
    stm_setup();
    
    state.pass_action = (sg_pass_action) {
        .colors[0] = { .action=SG_ACTION_CLEAR, .value={0.38f, 0.6f, 1.f, 1.f} }
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
    
    snk_setup(&(snk_desc_t) {});
    
    state.rng = NewRandom(0);
    state.world = EcsNewWorld();
    InitMap();
    state.zoom = 1.f;
    
    EcsPositionComponent = ECS_COMPONENT(state.world, Vec2);
    EcsTargetComponent = ECS_COMPONENT(state.world, Vec2);
    EcsNPC = ECS_TAG(state.world);
    EcsTextureBatchComponent = ECS_COMPONENT(state.world, TextureBatch);
    
    EcsCamera = ECS_TAG(state.world);
    Entity view = EcsNewEntity(state.world);
    EcsName(state.world, view, "Camera");
    EcsAttach(state.world, view, EcsPositionComponent);
    Position *viewPosition = EcsGet(state.world, view, EcsPositionComponent);
    *viewPosition = (Vec2){CHUNK_WIDTH * TILE_WIDTH / 2, CHUNK_HEIGHT * HALF_TILE_HEIGHT / 2};
    EcsAttach(state.world, view, EcsTargetComponent);
    Vec2 *viewTarget = EcsGet(state.world, view, EcsTargetComponent);
    *viewTarget = *viewPosition;
    EcsAttach(state.world, view, EcsCamera);
    
    state.textures = NewTextureManager();
    TextureManagerAdd(&state.textures, "assets/tiles.png");
    
    state.chunkTiles = NewTextureBatch(TextureManagerGet(&state.textures, "assets/tiles.png"), CHUNK_SIZE * 6);
    
    ECS_SYSTEM(state.world, UpdateCamera, EcsCamera);
}

static void frame(void) {
    state.delta = (float)(sapp_frame_duration() * 60.0);
    
    struct nk_context *ctx = snk_new_frame();
    
    sg_begin_default_pass(&state.pass_action, sapp_width(), sapp_height());
    sg_apply_pipeline(state.pipeline);
    EcsStep(state.world);
    CommitTextureBatch(&state.chunkTiles);
    snk_render(sapp_width(), sapp_height());
    sg_end_pass();
    sg_commit();
    
    ResetInputHandler();
}

static void cleanup(void) {
    if (state.map.heightmap)
        free(state.map.heightmap);
    DestroyTextureBatch(&state.chunkTiles);
    DestroyTextureManager(&state.textures);
    DestroyWorld(&state.world);
    snk_shutdown();
    sg_shutdown();
}

static void event(const sapp_event *e) {
    snk_handle_event(e);
    InputHandler(e);
    switch (e->type) {
        case SAPP_EVENTTYPE_MOUSE_DOWN:
            if (e->mouse_button == SAPP_MOUSEBUTTON_LEFT)
                state.mouseDownPos = (Vec2){ e->mouse_x, e->mouse_y };
            break;
        case SAPP_EVENTTYPE_MOUSE_UP:
            if (e->mouse_button == SAPP_MOUSEBUTTON_LEFT)
                ECS_QUERY(state.world, UpdateCameraTarget, NULL, EcsPositionComponent, EcsTargetComponent);
            break;
        default:
            break;
    }
}

sapp_desc sokol_main(int argc, char* argv[]) {
    sargs_setup(&(sargs_desc) {.argv=argv, .argc=argc});
    return (sapp_desc) {
        .init_cb = init,
        .frame_cb = frame,
        .event_cb = event,
        .cleanup_cb = cleanup,
        .window_title = "colony",
        .width = 640,
        .height = 480,
        .icon.sokol_default = true
    };
}
