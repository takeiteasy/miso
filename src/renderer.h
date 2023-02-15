//
//  renderer.h
//  colony
//
//  Created by George Watson on 09/02/2023.
//

#ifndef renderer_h
#define renderer_h
#include "linalgb.h"
#include "bitmap.h"

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
    sg_bindings bind;
    Vec2 size;
} TextureBatch;

typedef struct RenderPass RenderPass;
typedef void(*RenderPassCb)(RenderPass*);
struct RenderPass {
    Vec2 size;
    sg_pass_action pass_action;
    sg_pipeline pip;
    RenderPassCb cb;
};

Texture LoadTexture(const char *path);
Texture NewTexture(int w, int h);
void DestroyTexture(Texture *texture);

TextureBatch NewTextureBatch(Texture *texture, int maxVertices);
void ResetTextureBatch(TextureBatch *batch) ;
void TextureBatchRender(TextureBatch *batch, Vec2 position, Vec2 size, Vec2 scale, float rotation, Rect clip);
void CommitTextureBatch(TextureBatch *batch);
void DestroyTextureBatch(TextureBatch *batch);

RenderPass NewRenderPass(int w, int h, RenderPassCb cb);
void RunRenderPass(RenderPass *pass);
void DestroyRenderPass(RenderPass *pass);

#endif /* renderer_h */
