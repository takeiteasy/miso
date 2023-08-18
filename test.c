#include "miso.h"
#include "assets/sprite.glsl.h"
#include "assets/framebuffer.glsl.h"

#define MAP_WIDTH 256
#define MAP_HEIGHT 256

static struct {
    Texture *texture;
    TextureBatch *batch;
} state;

static void init(void) {
    Image *test = LoadImageFromFile("assets/tiles.png");
    state.texture = LoadTextureFromImage(test);
    state.batch = CreateTextureBatch(state.texture, 2);
    DestroyImage(test);
}

static void frame(void) {
    DrawTexture(state.texture, (Vector2){0, 0}, (Vector2){state.texture->w, state.texture->h}, (Vector2){1.f, 1.f}, (Vector2){640, 480}, 1.f, (Rectangle){0, 0, state.texture->w, state.texture->h});
    TextureBatchDraw(state.batch, (Vector2){50, 50}, (Vector2){state.texture->w, state.texture->h}, (Vector2){1.f, 1.f}, (Vector2){640, 480}, 1.f, (Rectangle){0, 0, state.texture->w, state.texture->h});
    TextureBatchDraw(state.batch, (Vector2){70, 70}, (Vector2){state.texture->w, state.texture->h}, (Vector2){1.f, 1.f}, (Vector2){640, 480}, 1.f, (Rectangle){0, 0, state.texture->w, state.texture->h});
    FlushTextureBatch(state.batch);
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
