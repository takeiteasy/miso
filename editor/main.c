//
//  main.c
//  editor
//
//  Created by George Watson on 28/08/2023.
//

#include "miso.h"
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_IMPLEMENTATION
#include "nuklear.h"
#define SOKOL_IMPL
#include "sokol_app.h"
#include "sokol_glue.h"
#include "sokol_nuklear.h"
#define JIM_IMPLEMENTATION
#include "jim.h"
#define MJSON_IMPLEMENTATION
#include "mjson.h"
#define OSDIALOG_IMPLEMENTATION
#include "osdialog.h"

#define MAP_SIZE 256
#define TILE_WIDTH 32.f
#define TILE_HEIGHT 16.f

static struct {
    sg_pass_action pass_action;
    Texture *texture;
    Chunk *map;
    Vector2 camera;
} state = {
    .pass_action.colors[0] = {
        .action=SG_ACTION_CLEAR,
        .value=(sg_color){0.f, 0.f, 0.f, 0.f}
    }
};

static void init(void) {
    sg_desc desc = {
        .context = sapp_sgcontext()
    };
    sg_setup(&desc);
    snk_desc_t snk_desc = {
        .depth_format = SG_PIXELFORMAT_DEPTH,
        .color_format = SG_PIXELFORMAT_RGBA8
    };
    snk_setup(&snk_desc);
    
    OrderMiso();
    state.camera = (Vector2){0.f, 0.f};
    state.texture = LoadTextureFromFile("assets/tiles.png");
    state.map = CreateChunk(state.texture, MAP_SIZE, MAP_SIZE, TILE_WIDTH, TILE_HEIGHT);
}

static void frame(void) {
    struct nk_context *ctx = snk_new_frame();
    if (nk_begin(ctx, "Settings", nk_rect(0, 0, 300, 600), NK_WINDOW_SCALABLE | NK_WINDOW_BORDER | NK_WINDOW_MINIMIZABLE)) {
    }
    nk_end(ctx);

    OrderUp(sapp_width(), sapp_height());
    DrawChunk(state.map, state.camera);
    snk_render(sapp_width(), sapp_height());
    FinishMiso();
    
    sg_commit();
}

static void event(const sapp_event *event) {
    
}

static void cleanup(void) {
    DestroyTexture(state.texture);
    DestroyChunk(state.map);
    CleanUpMiso();
    sg_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[]) {
    return (sapp_desc){
        .init_cb = init,
        .frame_cb = frame,
        .event_cb = event,
        .cleanup_cb = cleanup,
        .width = 800,
        .height = 600,
        .window_title = "miso editor",
        .icon.sokol_default = true
    };
}
