//
//  basic.c
//  miso
//
//  Created by George Watson on 18/08/2023.
//

#include "miso.h"
#define SOKOL_IMPL
#include "sokol_app.h"
#include "sokol_glue.h"

static struct {
    Texture *texture;
    Chunk *map;
    Vector2 camera;
} state;

#define MAP_SIZE 256
#define TILE_WIDTH 32.f
#define TILE_HEIGHT 16.f

static void init(void) {
    sg_desc desc = {
        .context = sapp_sgcontext()
    };
    sg_setup(&desc);
    
    OrderMiso(sapp_width(), sapp_height());
    
    state.camera = (Vector2){0.f, 0.f};
    state.texture = LoadTextureFromFile("assets/tiles.png");
    state.map = CreateMap(state.texture, MAP_SIZE, MAP_SIZE, TILE_WIDTH, TILE_HEIGHT);
}

static void frame(void) {
    OrderUp();
    DrawChunk(state.map, state.camera);
    FinishMiso();
    sg_commit();
}

static void cleanup(void) {
    DestroyTexture(state.texture);
    DestroyChunk(state.map);
    CleanUpMiso();
    sg_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[]) {
    return (sapp_desc) {
        .width = 640,
        .height = 480,
        .window_title = "miso -- basic.c",
        .init_cb = init,
        .frame_cb = frame,
        .cleanup_cb = cleanup
    };
}
