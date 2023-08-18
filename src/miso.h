//
//  miso.h
//  miso
//
//  Created by George Watson on 17/08/2023.
//

#ifndef miso_h
#define miso_h

#if defined(_MSC_VER) && _MSC_VER < 1800
#include <windef.h>
#define bool BOOL
#define true 1
#define false 0
#else
#if defined(__STDC__) && __STDC_VERSION__ < 199901L
typedef enum bool { false = 0, true = !false } bool;
#else
#include <stdbool.h>
#endif
#endif
#include <stddef.h>
#include <time.h>
#include <stdlib.h>
#include <float.h>
#include <math.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <setjmp.h>

#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"

#if defined(__EMSCRIPTEN__) || defined(EMSCRIPTEN)
#include <emscripten.h>
#define MISO_EMSCRIPTEN
#endif

#define MISO_POSIX
#if defined(macintosh) || defined(Macintosh) || (defined(__APPLE__) && defined(__MACH__))
#define MISO_MAC
#elif defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__WINDOWS__)
#define MISO_WINDOWS
#if !defined(MISO_FORCE_POSIX)
#undef MISO_POSIX
#endif
#elif defined(__gnu_linux__) || defined(__linux__) || defined(__unix__)
#define MISO_LINUX
#else
#error "Unsupported operating system"
#endif

#if defined(MISO_POSIX)
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#define PATH_SEPERATOR "/"
#if defined(MISO_MAC)
#define MAX_PATH 255
#else
#define MAX_PATH 4096
#endif
#else // Windows
#include <io.h>
#define F_OK    0
#define access _access
#define PATH_SEPERATOR "\\"
#define MAX_PATH 256
#endif

typedef struct {
    float x, y;
} Vector2;

typedef struct {
    float x, y, z, w;
} Vector4;

typedef struct {
    float x, y, w, h;
} Rectangle;

typedef union {
    struct {
        unsigned char a, b, g, r;
    };
    int rgba;
} Color;

typedef struct {
    int *buf, w, h;
} Image;

typedef struct {
    Vector2 position, texcoord;
    Vector4 color;
} Vertex;

typedef struct {
    sg_image sg;
    int w, h;
} Texture;

typedef struct {
    Vertex *vertices;
    int maxVertices, vertexCount;
    sg_bindings bind;
    Vector2 size;
} TextureBatch;

int OrderUp(const sapp_desc *desc);

int WindowWidth(void);
int WindowHeight(void);
int IsWindowFullscreen(void);
void ToggleFullscreen(void);
int IsCursorVisible(void);
void ToggleCursorVisible(void);
int IsCursorLocked(void);
void ToggleCursorLock(void);
void SetClearColor(Color color);

bool IsKeyDown(sapp_keycode key);
bool IsKeyUp(sapp_keycode key);
bool WasKeyPressed(sapp_keycode key);
bool IsButtonDown(sapp_mousebutton button);
bool IsButtonUp(sapp_mousebutton button);
bool WasButtonPressed(sapp_mousebutton button);
bool WasMouseScrolled(void);
bool WasMouseMoved(void);
Vector2 MousePosition(void);
Vector2 LastMousePosition(void);
Vector2 MouseScrollDelta(void);
Vector2 MouseMoveDelta(void);

bool DoesFileExist(const char *path);
bool DoesDirExist(const char *path);
char* FormatString(const char *fmt, ...);
char* LoadFile(const char *path, size_t *length);
const char* JoinPath(const char *a, const char *b);

Image* CreateImage(unsigned int w, unsigned int h);
void DestroyImage(Image *img);
void ImageSet(Image *img, int x, int y, Color col);
Color ImageGet(Image *img, int x, int y);
Image* LoadImageFromFile(const char *path);
Image* LoadImageFromMemory(const void *data, size_t length);
bool SaveImage(Image *img, const char *path);

Texture* LoadTextureFromImage(Image *img);
Texture* LoadTextureFromFile(const char *path);
Texture* CreateMutableTexture(int w, int h);
void UpdateMutableTexture(Texture *texture, Image *img);
void DrawTexture(Texture *texture, Vector2 position, Vector2 size, Vector2 scale, Vector2 viewportSize, float rotation, Rectangle clip);
void DestroyTexture(Texture *texture);

TextureBatch* CreateTextureBatch(Texture *texture, int maxVertices);
void TextureBatchDraw(TextureBatch *batch, Vector2 position, Vector2 size, Vector2 scale, Vector2 viewportSize, float rotation, Rectangle clip);
void FlushTextureBatch(TextureBatch *batch);
void DestroyTextureBatch(TextureBatch *batch);

float Perlin(float x, float y, float z);
unsigned char* PerlinFBM(int w, int h, float xoff, float yoff, float z, float scale, float lacunarity, float gain, int octaves);

#endif /* miso_h */
