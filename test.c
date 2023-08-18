#include "miso.h"

typedef struct {
    Vector2 position;
} Cell;

typedef struct {
    TextureBatch *batch;
    Cell *grid;
    int w, h;
} Map;

typedef struct {
    Vector2 position, size;
} Camera;

static struct {
    Texture *tiles;
    Map *map;
    Camera camera;
} state;

#define MAP_SIZE 256

Map* CreateMap(Texture *texture, int w, int h) {
    Map *result = malloc(sizeof(Map));
    result->batch = CreateTextureBatch(texture, w * h);
    result->grid = malloc(w * h * sizeof(Cell));
    result->w = w;
    result->h = h;
    return result;
}

void DrawMap(Map *map, Camera *camera) {
    Vector2 offset = {
        .x = 16 + (-camera->position.x + camera->size.x / 2),
        .y = 8  + (-camera->position.y + camera->size.y / 2)
    };
    for (int x = 0; x < MAP_SIZE; x++)
        for (int y = 0; y < MAP_SIZE; y++) {
            Vector2 p = (Vector2) {
                offset.x + ((float)x * 32.f) + (y % 2 ? 16 : 0),
                offset.y + ((float)y * 16.f) - (y * 8)
            };
            TextureBatchDraw(map->batch, (Vector2){p.x - 16.f, p.y - 16.f}, (Vector2){32.f, 32.f}, (Vector2){1.f, 1.f}, camera->size, 0.f, (Rectangle){0, 0, 32, 32});
        }
    FlushTextureBatch(map->batch);
}

void DestroyMap(Map *map) {
    if (map) {
        if (map->batch)
            DestroyTextureBatch(map->batch);
        if (map->grid)
            free(map->grid);
        free(map);
    }
}

static void init(void) {
    state.camera = (Camera) {
        .position = (Vector2){0, 0},
        .size = (Vector2){640, 480}
    };
    state.tiles = LoadTextureFromFile("assets/tiles.png");
    state.map = CreateMap(state.tiles, MAP_SIZE, MAP_SIZE);
}

static void frame(void) {
    DrawMap(state.map, &state.camera);
}

static void event(const sapp_event *event) {
    
}

static void cleanup(void) {
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
