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

#if defined(MISO_WINDOWS)
#include <windows.h>
#define getcwd _getcwd
#else
#include <pwd.h>
#endif

#if !defined(MAX_PATH)
#if defined(MISO_MAC)
#define MAX_PATH 255
#elif defined(MISO_WINDOWS)
#define MAX_PATH 256
#elif defined(MISO_LINUX)
#define MAX_PATH 4096
#endif
#endif

#if defined(MISO_POSIX)
#define PATH_SEPERATOR '/'
#else
#define PATH_SEPERATOR '\\'
#endif

#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define CLAMP(N, MI, MA) (MIN(MAX((N), (MI)), (MA)))

static const char* CurrentDirectory(void) {
    static char buf[MAX_PATH];
    memset(buf, 0, MAX_PATH * sizeof(char));
    getcwd(buf, MAX_PATH);
    size_t length = strlen(buf);
    if (buf[length-1] != PATH_SEPERATOR)
        buf[length] = PATH_SEPERATOR;
    return buf;
}

typedef struct {
    int mapWidth, mapHeight;
    int tileWidth, tileHeight;
} Settings;

static struct {
    sg_pass_action pass_action;
    MisoTexture *gridTexture, *mapTexture;
    MisoChunk *grid, *map;
    MisoCamera camera;
    float cameraSpeed;
    float cameraScrollSpeed;
    bool dragging;
    MisoVec2 lastMousePos, mousePos;
    float scrollY;
    Settings settings;
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
    
    state.settings.mapWidth = 128;
    state.settings.mapHeight = 128;
    state.settings.tileWidth = 32;
    state.settings.tileHeight = 16;
    state.cameraSpeed = 2.f;
    state.cameraScrollSpeed = .5f;
    
    OrderMiso();
    state.camera = (MisoCamera) {
        .position = (MisoVec2){0.f, 0.f},
        .zoom = 1.f
    };
    state.mapTexture = MisoLoadTextureFromFile("assets/tiles.png");
    state.map = MisoEmptyChunk(state.mapTexture, state.settings.mapWidth, state.settings.mapHeight, state.settings.tileWidth, state.settings.tileHeight);
    state.gridTexture = MisoLoadTextureFromFile("assets/grid.png");
    state.grid = MisoEmptyChunk(state.gridTexture, state.settings.mapWidth, state.settings.mapHeight, state.settings.tileWidth, state.settings.tileHeight);
}

static void frame(void) {
    struct nk_context *ctx = snk_new_frame();
    Settings tmp;
    memcpy(&tmp, &state.settings, sizeof(Settings));
    if (nk_begin(ctx, "Settings", nk_rect(0, 0, 300, 600), NK_WINDOW_SCALABLE | NK_WINDOW_BORDER | NK_WINDOW_MINIMIZABLE | NK_WINDOW_MOVABLE)) {
        if (nk_tree_push(ctx, NK_TREE_TAB, "Map", NK_MAXIMIZED)) {
            nk_property_int(ctx, "#Map Width:", 32, &tmp.mapWidth, 1024, 1, 1);
            nk_property_int(ctx, "#Map Height:", 32, &tmp.mapHeight, 1024, 1, 1);
//            nk_property_int(ctx, "#Tile Width:", 8, &tmp.tileWidth, 64, 1, 1);
//            nk_property_int(ctx, "#Tile Height:", 8, &tmp.tileHeight, 64, 1, 1);
            nk_tree_pop(ctx);
        }
        float tmpCameraSpeed = state.cameraSpeed;
        float tmpCameraZoom = state.camera.zoom;
        float tmpCameraScroll = state.cameraScrollSpeed;
        if (nk_tree_push(ctx, NK_TREE_TAB, "Camera", NK_MINIMIZED)) {
            static char buffer[64];
            sprintf(buffer, "Camera X: %2.f", state.camera.position.x);
            nk_label(ctx, buffer, NK_TEXT_ALIGN_LEFT);
            sprintf(buffer, "Camera Y: %2.f", state.camera.position.y);
            nk_label(ctx, buffer, NK_TEXT_ALIGN_LEFT);
            nk_property_float(ctx, "#Zoom:", .1f, &tmpCameraZoom, 10.f, .1f, .1f);
            nk_property_float(ctx, "#Drag Speed:", .1f, &tmpCameraSpeed, 10.f, .1f, .1f);
            nk_property_float(ctx, "#Scroll Speed:", .1f, &tmpCameraScroll, 10.f, .1f, .1f);
            nk_tree_pop(ctx);
        }
        state.cameraSpeed = tmpCameraSpeed;
        state.camera.zoom = tmpCameraZoom;
        state.cameraScrollSpeed = tmpCameraScroll;
        if (nk_tree_push(ctx, NK_TREE_TAB, "Tiles", NK_MAXIMIZED)) {
            nk_label(ctx, "Current Map Texture:", NK_TEXT_ALIGN_LEFT);
            struct nk_image img = nk_image_id((int)state.mapTexture->sg.id);
            nk_layout_row_static(ctx, state.mapTexture->h, state.mapTexture->w, 1);
            nk_image(ctx, img);
            if (nk_button_label(ctx, "Load tile map...")) {
                osdialog_filters *filter = osdialog_filters_parse("Images:jpg,jpeg,png,tga,bmp,qoi");
                char *out = osdialog_file(OSDIALOG_OPEN, CurrentDirectory(), NULL, filter);
                if (out) {
                    free(out);
                }
                osdialog_filters_free(filter);
            }
            
            nk_tree_pop(ctx);
        }
    }
    
    if (!nk_window_is_hovered(ctx)) {
        if (state.dragging) {
            state.camera.position.x -= (state.mousePos.x - state.lastMousePos.x) * state.cameraSpeed;
            state.camera.position.y -= (state.mousePos.y - state.lastMousePos.y) * state.cameraSpeed;
        }
        float newZoom = state.camera.zoom + state.scrollY * state.cameraScrollSpeed;
        state.camera.zoom = CLAMP(newZoom, .1f, 10.f);
    }
    nk_end(ctx);

    OrderUp(sapp_width(), sapp_height());
    MisoDrawChunk(state.map, &state.camera);
    MisoDrawChunk(state.grid, &state.camera);
    snk_render(sapp_width(), sapp_height());
    FinishMiso();
    
    sg_commit();
    memcpy(&state.settings, &tmp, sizeof(Settings));
    state.scrollY = 0.f;
    state.lastMousePos = state.mousePos;
}

static void event(const sapp_event *e) {
    snk_handle_event(e);
    switch (e->type) {
        case SAPP_EVENTTYPE_KEY_DOWN:
#if defined(MISO_MAC)
            if (e->modifiers & SAPP_MODIFIER_SUPER && (e->key_code == SAPP_KEYCODE_W || e->key_code == SAPP_KEYCODE_Q))
                sapp_quit();
#else
            if (e->modifiers & SAPP_MODIFIER_ALT && e->key_code == SAPP_KEYCODE_F4)
                sapp_quit();
#endif
            break;
        case SAPP_EVENTTYPE_MOUSE_SCROLL:
            state.scrollY = e->scroll_y;
            break;
        case SAPP_EVENTTYPE_MOUSE_DOWN:
        case SAPP_EVENTTYPE_MOUSE_UP:
            state.dragging = e->mouse_button == SAPP_MOUSEBUTTON_LEFT && e->type == SAPP_EVENTTYPE_MOUSE_DOWN;
            break;
        case SAPP_EVENTTYPE_MOUSE_MOVE:
            state.mousePos = (MisoVec2){e->mouse_x, e->mouse_y};
            break;
        default:
            break;
    }
}

static void cleanup(void) {
    MisoDestroyTexture(state.mapTexture);
    MisoDestroyTexture(state.gridTexture);
    MisoDestroyChunk(state.map);
    MisoDestroyChunk(state.grid);
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
