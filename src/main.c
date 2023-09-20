//
//  main.c
//  editor
//
//  Created by George Watson on 28/08/2023.
//

#include "miso.h"
#include "lua.h"
#include "ecs.h"
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
#include "font8x8_basic.h"
#if defined(MISO_WINDOWS)
#include <windows.h>
#define getcwd _getcwd
#else
#include <pwd.h>
#include <unistd.h>
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

#if !defined(MIN)
#define MIN(A, B) ((A) < (B) ? (A) : (B))
#endif
#if !defined(MAX)
#define MAX(A, B) ((A) > (B) ? (A) : (B))
#endif
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
    MisoVec2 mouseGridPos;
    float scrollY;
    Settings settings;
    MisoTextureBatch *fontBatch;
    MisoTexture *fontTexture;
} state = {
    .pass_action.colors[0] = {
        .action=SG_ACTION_CLEAR,
        .value=(sg_color){0.f, 0.f, 0.f, 0.f}
    }
};

static void LuaLoadMiso(lua_State *L) {
    // TODO: Load miso wrapper
}

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
    
    InitEcsWorld();
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    LuaLoadEcs(L);
    LuaLoadMiso(L);
    lua_pushcfunction(L, LuaDumpTable);
    lua_setglobal(L, "LuaDumpTable");
    lua_pushcfunction(L, LuaDumpStack);
    lua_setglobal(L, "LuaDumpStack");
    if (luaL_dofile(L, "assets/test.lua")) {
        fprintf(stderr, "ERROR: %s\n", lua_tostring(L, -1));
        assert(0);
    }
    
    OrderMiso();
    state.camera = (MisoCamera) {
        .position = (MisoVec2){0.f, 0.f},
        .zoom = 1.f
    };
    state.mapTexture = MisoLoadTextureFromFile("assets/default.png");
    state.map = MisoEmptyChunk(state.mapTexture, state.settings.mapWidth, state.settings.mapHeight, state.settings.tileWidth, state.settings.tileHeight);
    state.gridTexture = MisoLoadTextureFromFile("assets/grid.png");
    state.grid = MisoEmptyChunk(state.gridTexture, state.settings.mapWidth, state.settings.mapHeight, state.settings.tileWidth, state.settings.tileHeight);
    MisoResizeTextureBatch(&state.grid->batch, (state.settings.mapWidth * state.settings.mapHeight) + 1);
    
    MisoImage *font = MisoEmptyImage(128 * 8, 8);
    MisoColor white = {.rgba = 0xFFFFFFFF};
    MisoColor black = {.a = 255};
#define DRAW_CHARACTER(C)                                                                  \
do {                                                                                       \
    for (int i = 0; i < 8; i++)                                                            \
        for (int j = 0; j < 8; j++)                                                        \
            MisoImagePSet(font, x + i, j, font8x8_basic[(C)][j] & 1 << i ? white : black); \
    x += 8;                                                                                \
} while(0)
#if !defined(MAX_FONT_VERTICES)
#define MAX_FONT_VERTICES 1024
#endif
    int x = 0;
    for (int c = 0; c < 128; c++)
        DRAW_CHARACTER(c);
    state.fontTexture = MisoLoadTextureFromImage(font);
    state.fontBatch = MisoCreateTextureBatch(state.fontTexture, MAX_FONT_VERTICES);
    MisoDestroyImage(font);
}

static void DrawMapGrid(MisoChunk *chunk, MisoCamera *camera, MisoVec2 position, MisoVec2 gridPosition) {
    MisoTextureBatchDraw(chunk->batch, (MisoVec2){position.x - (chunk->tileW / 2), position.y - (chunk->tileH / 2)}, (MisoVec2){chunk->tileW, chunk->tileH}, (MisoVec2){camera->zoom, camera->zoom}, (MisoVec2){sapp_width(), sapp_height()}, 0.f, (MisoRect){0, 0, chunk->tileW, chunk->tileH});
}

void DbgDrawString(int x, int y, const char *string) {
    int xoff = x, yoff = y + 25;
    for (int i = 0; i < strlen(string); i++) {
        switch (string[i]) {
            case '\n':
                yoff += 8;
                xoff  = x;
                break;
            default:
                MisoTextureBatchDraw(state.fontBatch, (MisoVec2){xoff, yoff}, (MisoVec2){8, 8}, (MisoVec2){1.f, 1.f}, (MisoVec2){sapp_width(), sapp_height()}, 0.f, (MisoRect){string[i] * 8, 0, 8, 8});
                xoff += 8;
                break;
        }
    }
}

#if !defined(_WIN32) && !defined(_WIN64)
// Taken from: https://stackoverflow.com/a/4785411
static int _vscprintf(const char *format, va_list pargs) {
    va_list argcopy;
    va_copy(argcopy, pargs);
    int retval = vsnprintf(NULL, 0, format, argcopy);
    va_end(argcopy);
    return retval;
}
#endif

void DbgDrawStringFormat(int x, int y, const char *format, ...) {
    va_list args;
    va_start(args, format);
    size_t size = _vscprintf(format, args) + 1;
    char *str = malloc(sizeof(char) * size);
    vsnprintf(str, size, format, args);
    va_end(args);
    DbgDrawString(x, y, str);
    free(str);
}

static void frame(void) {
    struct nk_context *ctx = snk_new_frame();
    Settings tmp;
    memcpy(&tmp, &state.settings, sizeof(Settings));
    if (nk_begin(ctx, "", nk_rect(0, 0, sapp_width(), 25), NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BACKGROUND)) {
        nk_menubar_begin(ctx);
        nk_layout_row_begin(ctx, NK_STATIC, 20, 1);
        nk_layout_row_push(ctx, 60);
        if (nk_menu_begin_label(ctx, "FILE", NK_TEXT_LEFT, nk_vec2(100, 600))) {
            nk_layout_row_dynamic(ctx, 20, 1);
            nk_menu_item_label(ctx, "New", NK_TEXT_LEFT);
            nk_menu_item_label(ctx, "Open", NK_TEXT_LEFT);
            nk_menu_item_label(ctx, "Save", NK_TEXT_LEFT);
            nk_menu_item_label(ctx, "Close", NK_TEXT_LEFT);
            nk_menu_item_label(ctx, "Exit", NK_TEXT_LEFT);
            nk_menu_end(ctx);
        }
        nk_menubar_end(ctx);
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
    MisoDrawChunkCustom(state.grid, &state.camera, DrawMapGrid);
    // TODO: World <-> Screen conversions need work, still not very accurate
    MisoVec2 mouseGridPosition = MisoChunkTileToScreen(state.grid, &state.camera, state.mouseGridPos);
    MisoTextureBatchDraw(state.grid->batch, (MisoVec2){mouseGridPosition.x - (state.grid->tileW / 2), mouseGridPosition.y - (state.grid->tileH / 2)}, (MisoVec2){state.grid->tileW, state.grid->tileH}, (MisoVec2){state.camera.zoom, state.camera.zoom}, (MisoVec2){sapp_width(), sapp_height()}, 0.f, (MisoRect){state.grid->tileW, 0, state.grid->tileW, state.grid->tileH});
    MisoFlushTextureBatch(state.grid->batch);
    DbgDrawString(0, 0, "Hello, world!");
    MisoFlushTextureBatch(state.fontBatch);
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
            state.mouseGridPos = MisoScreenToChunkTile(state.map, &state.camera, state.mousePos);
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
    MisoDestroyTexture(state.fontTexture);
    MisoDestroyTextureBatch(state.fontBatch);
    CleanUpMiso();
    snk_shutdown();
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
        .window_title = "misoEngine",
        .icon.sokol_default = true
    };
}
