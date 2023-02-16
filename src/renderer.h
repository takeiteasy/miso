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
#include "hashmap.h"

#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sprite.glsl.h"

#define NoColor (sg_color){0.f,0.f,0.f,1.f}

typedef struct {
    Vec2 position, texcoord;
    sg_color color;
} Vertex;

typedef struct {
    Vec2 size;
    sg_image ref;
} Texture;

typedef struct {
    Vertex *vertices;
    int maxVertices, vertexCount;
    sg_bindings binding;
    Texture texture;
} TextureBatch;

typedef struct hashmap Hashmap;
typedef struct RenderPass RenderPass;
typedef void(*RenderPassCb)(RenderPass*);

typedef struct {
    const char *name;
    TextureBatch batch;
} TextureBucket;

struct RenderPass {
    Vec2 size;
    sg_pass_action pass_action;
    sg_pipeline pipeline;
    Hashmap *textures;
};

RenderPass NewRenderPass(int w, int h);
void RenderPassBegin(RenderPass *pass);
void RenderPassEnd(RenderPass *pass);
void RenderPassNewBatch(RenderPass *pass, const char *path, int maxVertices);
TextureBatch* RenderPassGetBatch(RenderPass *pass, const char *path);
void TextureBatchRender(TextureBatch *batch, Vec2 position, Vec2 size, Vec2 scale, Vec2 viewportSize, float rotation, Rect clip);
void DestroyRenderPass(RenderPass *pass);

#endif /* renderer_h */
