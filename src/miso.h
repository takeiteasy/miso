//
//  miso.h
//  miso
//
//  Created by George Watson on 17/08/2023.
//

#ifndef miso_h
#define miso_h
#if defined(__cplusplus)
extern "C" {
#endif

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
#include <assert.h>
#include "sokol_gfx.h"
#include "stb_image.h"
#include "qoi.h"

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

#if defined(MISO_WINDOWS) && !defined(MISO_NO_EXPORT)
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
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
        unsigned char r, g, b, a;
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

typedef struct {
    TextureBatch *batch;
    int *grid;
    float tileW, tileH;
    int w, h;
} Chunk;

EXPORT Chunk* CreateChunk(Texture *texture, int w, int h, int tw, int th);
EXPORT int ChunkAt(Chunk *chunk, int x, int y);
EXPORT void ChunkSet(Chunk *chunk, int x, int y, int value);
EXPORT void DrawChunk(Chunk *chunk, Vector2 cameraPosition);
EXPORT void DestroyChunk(Chunk *chunk);

EXPORT Image* CreateImage(unsigned int w, unsigned int h);
EXPORT void DestroyImage(Image *img);
EXPORT void ImageSet(Image *img, int x, int y, Color col);
EXPORT Color ImageGet(Image *img, int x, int y);
EXPORT Image* LoadImageFromFile(const char *path);
EXPORT Image* LoadImageFromMemory(const void *data, size_t length);

EXPORT Texture* LoadTextureFromImage(Image *img);
EXPORT Texture* LoadTextureFromFile(const char *path);
EXPORT Texture* CreateEmptyTexture(int w, int h);
EXPORT void UpdateTexture(Texture *texture, Image *img);
EXPORT void DrawTexture(Texture *texture, Vector2 position, Vector2 size, Vector2 scale, Vector2 viewportSize, float rotation, Rectangle clip);
EXPORT void DestroyTexture(Texture *texture);

EXPORT TextureBatch* CreateTextureBatch(Texture *texture, int maxVertices);
EXPORT void TextureBatchDraw(TextureBatch *batch, Vector2 position, Vector2 size, Vector2 scale, Vector2 viewportSize, float rotation, Rectangle clip);
EXPORT void FlushTextureBatch(TextureBatch *batch);
EXPORT void DestroyTextureBatch(TextureBatch *batch);

EXPORT void OrderMiso(void);
EXPORT void OrderUp(unsigned int width, unsigned int height);
EXPORT void FinishMiso(void);
EXPORT void CleanUpMiso(void);

#if defined(__cplusplus)
}
#endif
#endif /* miso_h */
