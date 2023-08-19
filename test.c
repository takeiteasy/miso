#include "miso.h"

typedef struct {
    Vector2 position, size;
} Camera;

static struct {
    Texture *texture;
    Chunk *map;
    Camera camera;
} state;

#define MAP_SIZE 256
#define TILE_WIDTH 32.f
#define TILE_HEIGHT 16.f

static void init(void) {
    state.camera = (Camera) {
        .position = (Vector2){0, 0},
        .size = (Vector2){640, 480}
    };
    state.texture = LoadTextureFromFile("assets/tiles.png");
    state.map = CreateMap(state.texture, MAP_SIZE, MAP_SIZE, TILE_WIDTH, TILE_HEIGHT);
}

static void frame(void) {
    DrawChunk(state.map, state.camera.position, state.camera.size);
}

static void event(const sapp_event *event) {
    
}

static void cleanup(void) {
    DestroyTexture(state.texture);
    DestroyChunk(state.map);
}

int main(int argc, const char *argv[]) {
    sapp_desc desc = {
        .width = 640,
        .height = 480,
        .window_title = "miso!",
        .init_cb = init,
        .frame_cb = frame,
        .event_cb = event,
        .cleanup_cb = cleanup
    };
    return OrderUp(&desc);
}
