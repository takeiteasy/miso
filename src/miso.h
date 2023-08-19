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

typedef struct {
    TextureBatch *batch;
    int *grid;
    Vector2 tileSize;
    int w, h;
} Chunk;

int OrderUp(const sapp_desc *desc);

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

Chunk* CreateMap(Texture *texture, int w, int h, int tw, int th);
int ChunkAt(Chunk *chunk, int x, int y);
void DrawChunk(Chunk *chunk, Vector2 cameraPosition, Vector2 viewportSize);
void DestroyChunk(Chunk *chunk);

float Perlin(float x, float y, float z);
unsigned char* PerlinFBM(int w, int h, float xoff, float yoff, float z, float scale, float lacunarity, float gain, int octaves);

#if defined(__cplusplus)
}
#endif
#endif /* miso_h */
