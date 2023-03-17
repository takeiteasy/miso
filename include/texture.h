//
//  texture.h
//  colony
//
//  Created by George Watson on 09/02/2023.
//

#ifndef texture_h
#define texture_h
#include "maths.h"
#include "bitmap.h"
#include "hashmap.h"

#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sprite.glsl.h"

#define NoColor (sg_color){0.f,0.f,0.f,1.f}

typedef struct {
    Vec2 position, texcoord;
} Vertex;

typedef sg_image Texture;

typedef struct {
    Vertex *vertices;
    Vec4 *colors;
    int maxVertices, vertexCount;
    sg_bindings binding;
    Vec2 size;
} TextureBatch;

Texture LoadTexture(const char *path);
Texture MutableTexture(int w, int h);
void DestroyTexture(Texture texture);

TextureBatch NewTextureBatch(Texture texture, int maxVertices);
void TextureBatchRender(TextureBatch *batch, Vec2 position, Vec2 size, Vec2 scale, Vec2 viewportSize, float rotation, Rect clip, Vec4 color);
void CommitTextureBatch(TextureBatch *batch);
void DestroyTextureBatch(TextureBatch *batch);

void InitTextureManager(void);
void TextureManagerAdd(const char *path);
Texture TextureManagerGet(const char *path);
void DestroyTextureManager(void);

#endif /* texture_h */
