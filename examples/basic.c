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
    MisoTexture *texture;
    MisoChunk *map;
    MisoVec2 camera;
} state;

#define MAP_SIZE 256
#define TILE_WIDTH 32.f
#define TILE_HEIGHT 16.f

static void init(void) {
    sg_desc desc = {
        .context = sapp_sgcontext()
    };
    sg_setup(&desc);
    
    OrderMiso();
    
    state.camera = (MisoVec2){0.f, 0.f};
    state.texture = MisoLoadTextureFromFile("assets/tiles.png");
    state.map = MisoEmptyChunk(state.texture, MAP_SIZE, MAP_SIZE, TILE_WIDTH, TILE_HEIGHT);
}

static void frame(void) {
    OrderUp(sapp_width(), sapp_height());
    MisoDrawChunk(state.map, state.camera);
    FinishMiso();
    sg_commit();
}

static void cleanup(void) {
    MisoDestroyTexture(state.texture);
    MisoDestroyChunk(state.map);
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
