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
} MisoVec2;

typedef struct {
    float x, y, z, w;
} MisoVec4;

typedef struct {
    float x, y, w, h;
} MisoRect;

typedef union {
    struct {
        unsigned char r, g, b, a;
    };
    int rgba;
} MisoColor;

typedef struct {
    int *buf, w, h;
} MisoImage;

typedef struct {
    MisoVec2 position, texcoord;
    MisoVec4 color;
} MisoVertex;

typedef struct {
    sg_image sg;
    int w, h;
} MisoTexture;

typedef struct {
    MisoVertex *vertices;
    int maxVertices, vertexCount;
    sg_bindings bind;
    MisoVec2 size;
} MisoTextureBatch;

typedef struct {
    MisoTextureBatch *batch;
    int *grid;
    float tileW, tileH;
    int w, h;
} MisoChunk;

typedef struct {
    MisoVec2 position;
    MisoVec2 size;
    float zoom;
} MisoCamera;

EXPORT MisoChunk* MisoEmptyChunk(MisoTexture *texture, int w, int h, int tw, int th);
EXPORT int MisoChunkAt(MisoChunk *chunk, int x, int y);
EXPORT void MisoChunkSet(MisoChunk *chunk, int x, int y, int value);
EXPORT void MisoDrawChunk(MisoChunk *chunk, MisoCamera *camera);
EXPORT void MisoDestroyChunk(MisoChunk *chunk);

EXPORT MisoImage* MisoEmptyImage(unsigned int w, unsigned int h);
EXPORT void MisoDestroyImage(MisoImage *img);
EXPORT void MisoImagePSet(MisoImage *img, int x, int y, MisoColor col);
EXPORT MisoColor MisoImagePGet(MisoImage *img, int x, int y);
EXPORT MisoImage* MisoLoadImageFromFile(const char *path);
EXPORT MisoImage* MisoLoadImageFromMemory(const void *data, size_t length);

EXPORT MisoTexture* MisoLoadTextureFromImage(MisoImage *img);
EXPORT MisoTexture* MisoLoadTextureFromFile(const char *path);
EXPORT MisoTexture* MisoEmptyTexture(int w, int h);
EXPORT void MisoUpdateTexture(MisoTexture *texture, MisoImage *img);
EXPORT void MisoDrawTexture(MisoTexture *texture, MisoVec2 position, MisoVec2 size, MisoVec2 scale, MisoVec2 viewportSize, float rotation, MisoRect clip);
EXPORT void MisoDestroyTexture(MisoTexture *texture);

EXPORT MisoTextureBatch* MisoCreateTextureBatch(MisoTexture *texture, int maxVertices);
EXPORT void MisoTextureBatchDraw(MisoTextureBatch *batch, MisoVec2 position, MisoVec2 size, MisoVec2 scale, MisoVec2 viewportSize, float rotation, MisoRect clip);
EXPORT void MisoFlushTextureBatch(MisoTextureBatch *batch);
EXPORT void MisoDestroyTextureBatch(MisoTextureBatch *batch);

EXPORT MisoVec2 MisoScreenToChunkTile(MisoChunk *chunk, MisoCamera *camera, MisoVec2 point);
EXPORT MisoVec2 MisoScreenToWorld(MisoCamera *camera, MisoVec2 point);
EXPORT MisoVec2 MisoWorldToScreen(MisoCamera *camera, MisoVec2 point);

EXPORT void OrderMiso(void);
EXPORT void OrderUp(unsigned int width, unsigned int height);
EXPORT void FinishMiso(void);
EXPORT void CleanUpMiso(void);

#if defined(__cplusplus)
}
#endif
#endif /* miso_h */
