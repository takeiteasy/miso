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

#include "gui.h"
#include "maths.h"
#include "ecs.h"
#include "texture.h"
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

//! TODO: Add velocity to camera drag
//! TODO: Cursor position to world/grid position
//! TODO: Lua integration
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
    bool isWindowHovered;
    
    Map map;
    Bitmap minimap;
    Texture minimapTexture;
    TextureBatch chunkTiles;
    Bitmap tileMask;
    
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

static int CalcMinimapTile(unsigned char height) {
    switch (MIN(255, height)) {
        case 26 ... 37:
            return RGB(11, 95, 230);
        case 38 ... 70:
            return RGB(212, 180, 63);
        case 71 ... 100:
            return RGB(83, 168, 37);
        case 101 ... 113:
            return RGB(34, 92, 18);
        default:
            return RGB(66, 135, 245);
    }
}

static void InitMap(void) {
    state.map.heightmap = PerlinFBM(CHUNK_WIDTH, CHUNK_HEIGHT, 0.f, 0.f, 0.f, 200.f, 2.f, .5f, 8);
    state.minimap = NewBitmap(CHUNK_WIDTH, CHUNK_HEIGHT);
    for (int x = 0; x < CHUNK_WIDTH; x++)
        for (int y = 0; y < CHUNK_HEIGHT; y++) {
            unsigned char h = state.map.heightmap[CHUNK_AT(x, y)];
            state.map.tiles[CHUNK_AT(x, y)] = CalcTile(h);
            state.minimap.buf[CHUNK_AT(x, y)] = CalcMinimapTile(h);
        }
    state.minimapTexture = MutableTexture(CHUNK_WIDTH, CHUNK_HEIGHT);
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

static Vec2 ScreenToWorld(Vec2 cameraPosition, Vec2 position) {
    float scale = Remap(state.zoom, 0.f, 2.f, 2.f, 0.f);
    Vec2 viewport = (Vec2){sapp_width(), sapp_height()} * scale;
    Vec2 cameraTopLeft = cameraPosition - (viewport / 2.f);
    int y  = position.y + HALF_TILE_HEIGHT + cameraTopLeft.y;
    int x  = position.x + HALF_TILE_WIDTH + cameraTopLeft.x;
    int gx = floor((x + TILE_WIDTH) / TILE_WIDTH) - 1;
    int gy = 2 * (floor((y + TILE_HEIGHT) / TILE_HEIGHT) - 1);
    int ox = x % TILE_WIDTH;
    int oy = y % TILE_HEIGHT;
    switch (PGet(&state.tileMask, ox, oy)) {
        case 0xFF0000FF:
            gy--;
            break;
        case 0xFFFF0000:
            gy++;
            break;
        case 0xFF00FFFF:
            gx--;
            gy--;
            break;
        case 0xFF00FF00:
            gx--;
            gy++;
            break;
    }
    return (Vec2){gx, gy};
}

static Vec2 MapToScreen(Vec2 cameraPosition, Vec2 position) {
    float scale = Remap(state.zoom, 0.f, 2.f, 2.f, 0.f);
    Vec2 viewport = (Vec2){sapp_width(), sapp_height()} * scale;
    Vec2 cameraTopLeft = cameraPosition - (viewport / 2.f);
    int gx = cameraTopLeft.x / TILE_WIDTH;
    int gy = cameraTopLeft.y / HALF_TILE_HEIGHT;
    float ox = cameraTopLeft.x - (gx * TILE_WIDTH);
    float oy = cameraTopLeft.y - (gy * HALF_TILE_HEIGHT);
    int dx = position.x - gx, dy = position.y - gy;
    int px = -ox + (dx * TILE_WIDTH) + ((int)position.y % 2 ? HALF_TILE_WIDTH : 0) - HALF_TILE_WIDTH;
    int py = -oy + (dy * HALF_TILE_HEIGHT) - HALF_TILE_HEIGHT;
    return (Vec2){px, py - 1};
}

static void UpdateCamera(Query *query) {
    Position *cameraPosition = EcsGet(state.world, query->entity, EcsPositionComponent);
    Vec2 *cameraTarget = EcsGet(state.world, query->entity, EcsTargetComponent);
    
    if (state.isWindowHovered)
        goto RENDER;

    if (WasMouseScrolled())
        state.zoom = CLAMP(state.zoom + MouseScrollDelta().y, .1f, 2.f);
    
    if (IsButtonDown(SAPP_MOUSEBUTTON_LEFT)) {
        *cameraTarget = *cameraPosition - MouseMoveDelta();
        *cameraPosition = MoveTowards(*cameraPosition, *cameraTarget, CAMERA_CHASE_SPEED * 20.f);
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
    
RENDER:
    RenderMap(*cameraPosition, (Vec2){sapp_width(), sapp_height()}, state.zoom);
}

struct nk_canvas {
    struct nk_command_buffer *painter;
    struct nk_vec2 item_spacing;
    struct nk_vec2 panel_padding;
    struct nk_style_item window_background;
};

static nk_bool canvas_begin(struct nk_context *ctx, struct nk_canvas *canvas, struct nk_color background_color) {
    /* save style properties which will be overwritten */
    canvas->panel_padding = ctx->style.window.padding;
    canvas->item_spacing = ctx->style.window.spacing;
    canvas->window_background = ctx->style.window.fixed_background;

    /* use the complete window space and set background */
    ctx->style.window.spacing = nk_vec2(0,0);
    ctx->style.window.padding = nk_vec2(0,0);
    ctx->style.window.fixed_background = nk_style_item_color(background_color);

    struct nk_rect total_space;
    total_space = nk_window_get_content_region(ctx);
    nk_layout_row_dynamic(ctx, total_space.h, 1);
    nk_widget(&total_space, ctx);
    canvas->painter = nk_window_get_canvas(ctx);
    
    return nk_true;
}

static void canvas_end(struct nk_context *ctx, struct nk_canvas *canvas) {
    ctx->style.window.spacing = canvas->panel_padding;
    ctx->style.window.padding = canvas->item_spacing;
    ctx->style.window.fixed_background = canvas->window_background;
}

static void RenderThickLine(Bitmap *b, int x, int y) {
    for (int xx = x - 1; xx < x + 2; xx++)
        for (int yy = y - 1; yy < y + 2; yy++)
            PSet(b, xx, yy, RGB(255, 0, 0));
}

static void RenderCameraView(Query *query) {
    Position *cameraPosition = EcsGet(state.world, query->entity, EcsPositionComponent);
    static Vec2 tile = (Vec2){TILE_WIDTH, HALF_TILE_HEIGHT};
    float scale = CLAMP(Remap(state.zoom, 0.f, 2.f, 2.f, 0.f), .1f, 2.f);
    Vec2 viewportSize = ((Vec2){sapp_width(),sapp_height()} / tile) * scale;
    Vec2 viewportHalfSize = viewportSize / 2.f;
    Vec2 cameraRelativePosition = *cameraPosition / tile;
    
    for (int x = 0; x < CHUNK_WIDTH; x++)
        for (int y = 0; y < CHUNK_HEIGHT; y++)
            state.minimap.buf[CHUNK_AT(x, y)] = CalcMinimapTile(state.map.heightmap[CHUNK_AT(x, y)]);
    for (int x = cameraRelativePosition.x - viewportHalfSize.x; x < cameraRelativePosition.x + viewportHalfSize.x; x++) {
        RenderThickLine(&state.minimap, x, cameraRelativePosition.y - viewportHalfSize.y);
        RenderThickLine(&state.minimap, x, cameraRelativePosition.y + viewportHalfSize.y);
    }
    for (int y = cameraRelativePosition.y - viewportHalfSize.y; y < cameraRelativePosition.y + viewportHalfSize.y; y++) {
        RenderThickLine(&state.minimap, cameraRelativePosition.x - viewportHalfSize.x, y);
        RenderThickLine(&state.minimap, cameraRelativePosition.x + viewportHalfSize.x, y);
    }
}

static void RenderMinimap(struct nk_context *ctx, int w, int h) {
    struct nk_canvas canvas;
    if (canvas_begin(ctx, &canvas, nk_rgb(250,250,250))) {
        ECS_QUERY(state.world, RenderCameraView, NULL, EcsCamera);
        sg_update_image(state.minimapTexture, &(sg_image_data) {
            .subimage[0][0] = {
                .ptr  = state.minimap.buf,
                .size = CHUNK_WIDTH * CHUNK_HEIGHT * sizeof(int)
            }
        });
        struct nk_image img = nk_image_id((int)state.minimapTexture.id);
        float x = canvas.painter->clip.x, y = canvas.painter->clip.y;
        nk_draw_image(canvas.painter, nk_rect(x, y, w, h), &img, nk_rgba(255, 255, 255, 255));
        canvas_end(ctx, &canvas);
    }
}

static void ForceCameraPosition(Query *query) {
    Position *cameraPosition = EcsGet(state.world, query->entity, EcsPositionComponent);
    *cameraPosition = *(Vec2*)query->userdata;
}

static void MinimapCallback(struct nk_context *ctx) {
    struct nk_vec2 size = nk_window_get_size(ctx);
    struct nk_vec2 pos = nk_window_get_position(ctx);
    RenderMinimap(ctx, size.x, size.y);
    if (nk_window_is_hovered(ctx) && IsButtonDown(SAPP_MOUSEBUTTON_LEFT)) {
        Vec2 mouseOriginal = MousePosition();
        Vec2 mouse = (Vec2) {
            mouseOriginal.x - pos.x,
            mouseOriginal.y - pos.y
        };
        static const int BORDER_WIDTH  = 8;
        static const int BORDER_HEIGHT = 32;
        if (mouse.x >= BORDER_WIDTH && mouse.y >= BORDER_HEIGHT && mouse.x < size.x + BORDER_WIDTH && mouse.y < size.y + BORDER_HEIGHT) {
            Vec2 position = (Vec2) {
                Remap(mouse.x - 8,  0, size.x, 0, CHUNK_WIDTH),
                Remap(mouse.y - 32, 0, size.y, 0, CHUNK_HEIGHT)
            } * (Vec2){TILE_WIDTH, HALF_TILE_HEIGHT};
            ECS_QUERY(state.world, ForceCameraPosition, (void*)&position, EcsCamera);
        }
    }
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
    state.zoom = 1.f;
    InitMap();
    state.tileMask = LoadBitmap("assets/mask.png");
    InitTextureManager();
        InitWindowManager();
    WindowManagerAdd("Minimap", nk_vec2(0.f, 0.f), nk_vec2(CHUNK_WIDTH/2,CHUNK_HEIGHT/2), false, NK_WINDOW_SCALABLE | NK_WINDOW_MOVABLE | NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER | NK_WINDOW_MINIMIZABLE | NK_WINDOW_CLOSABLE, MinimapCallback, SAPP_KEYCODE_M);

    
    EcsPositionComponent = ECS_COMPONENT(state.world, Vec2);
    EcsTargetComponent = ECS_COMPONENT(state.world, Vec2);
    EcsNPC = ECS_TAG(state.world);
    EcsTextureBatchComponent = ECS_COMPONENT(state.world, TextureBatch);
    
    EcsCamera = ECS_TAG(state.world);
    Entity view = EcsNewEntity(state.world);
    EcsName(state.world, view, "Camera");
    EcsAttach(state.world, view, EcsPositionComponent);
    Position *viewPosition = EcsGet(state.world, view, EcsPositionComponent);
//    *viewPosition = (Vec2){0.f,0.f};
    *viewPosition = (Vec2){CHUNK_WIDTH * TILE_WIDTH / 2, CHUNK_HEIGHT * HALF_TILE_HEIGHT / 2};
    EcsAttach(state.world, view, EcsTargetComponent);
    Vec2 *viewTarget = EcsGet(state.world, view, EcsTargetComponent);
    *viewTarget = *viewPosition;
    EcsAttach(state.world, view, EcsCamera);
    
    TextureManagerAdd("assets/tiles.png");
    
    state.chunkTiles = NewTextureBatch(TextureManagerGet("assets/tiles.png"), CHUNK_SIZE * 6);
    
    ECS_SYSTEM(state.world, UpdateCamera, EcsCamera);
}

static void frame(void) {
    state.delta = (float)(sapp_frame_duration() * 60.0);
    
    struct nk_context *ctx = snk_new_frame();
    WindowManagerUpdate(ctx);
    state.isWindowHovered = nk_window_is_any_hovered(ctx);
    
    Vec2 test = MouseScrollDelta();
    printf("%f, %f\n", test.x, test.y);
    
    sg_begin_default_pass(&state.pass_action, sapp_width(), sapp_height());
    sg_apply_pipeline(state.pipeline);
    EcsStep(state.world);
    CommitTextureBatch(&state.chunkTiles);
    snk_render(sapp_width(), sapp_height());
    sg_end_pass();
    sg_commit();
    ResetInput();
}

static void cleanup(void) {
    free(state.map.heightmap);
    DestroyBitmap(&state.tileMask);
    DestroyBitmap(&state.minimap);
    DestroyTexture(state.minimapTexture);
    DestroyTextureBatch(&state.chunkTiles);
    DestroyTextureManager();
    DestroyWindowManager();
    DestroyWorld(&state.world);
    snk_shutdown();
    sg_shutdown();
}

static void event(const sapp_event *e) {
    snk_handle_event(e);
    InputHandler(e);
    switch (e->type) {
        case SAPP_EVENTTYPE_KEY_DOWN:
#if defined(DEBUG)
            if (e->key_code == SAPP_KEYCODE_ESCAPE && sapp_is_fullscreen())
                sapp_toggle_fullscreen();
            if (e->modifiers & SAPP_MODIFIER_SUPER && (e->key_code == SAPP_KEYCODE_W || e->key_code == SAPP_KEYCODE_Q))
                sapp_quit();
#endif
            break;
        case SAPP_EVENTTYPE_KEY_UP:
            break;
        case SAPP_EVENTTYPE_MOUSE_DOWN:
            if (e->mouse_button == SAPP_MOUSEBUTTON_LEFT) {
                state.mouseDownPos = (Vec2){ e->mouse_x, e->mouse_y };
                ECS_QUERY(state.world, UpdateCameraTarget, NULL, EcsCamera);
            }
            break;
        case SAPP_EVENTTYPE_MOUSE_UP:
            if (e->mouse_button == SAPP_MOUSEBUTTON_LEFT)
                ECS_QUERY(state.world, UpdateCameraTarget, NULL, EcsCamera);
            break;
        case SAPP_EVENTTYPE_MOUSE_MOVE:
            break;
        case SAPP_EVENTTYPE_MOUSE_SCROLL:
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
