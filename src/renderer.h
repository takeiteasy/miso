//
//  renderer.h
//  colony
//
//  Created by George Watson on 09/02/2023.
//

#ifndef renderer_h
#define renderer_h
#include "maths.h"
#include "bitmap.h"
#include "ecs.h"
#include "hashmap.h"

#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sprite.glsl.h"

extern Entity EcsTextureBatchComponent;

#define NoColor (sg_color){0.f,0.f,0.f,1.f}

typedef struct {
    Vec2 position, texcoord;
} Vertex;

typedef sg_image Texture;

typedef struct {
    Vertex *vertices;
    int maxVertices, vertexCount;
    sg_bindings binding;
    Vec2 size;
} TextureBatch;

typedef struct {
    struct hashmap *map;
} TextureManager;

Texture LoadTexture(const char *path);
void DestroyTexture(Texture texture);

TextureBatch NewTextureBatch(Texture texture, int maxVertices);
void TextureBatchRender(TextureBatch *batch, Vec2 position, Vec2 size, Vec2 scale, Vec2 viewportSize, float rotation, Rect clip);
void CommitTextureBatch(TextureBatch *batch);
void DestroyTextureBatch(TextureBatch *batch);

TextureManager NewTextureManager(void);
void TextureManagerAdd(TextureManager *manager, const char *path);
Texture TextureManagerGet(TextureManager *manager, const char *path);
void DestroyTextureManager(TextureManager *manager);

#endif /* renderer_h */
