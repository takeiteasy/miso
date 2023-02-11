//
//  sprite.h
//  colony
//
//  Created by George Watson on 09/02/2023.
//

#ifndef sprite_h
#define sprite_h
#include "sokol_gfx.h"
#include "linalgb.h"
#include "bitmap.h"
#include "sprite.glsl.h"

#define NoColor (Vec4){0.f,0.f,0.f,1.f}

typedef struct {
    int w, h;
    sg_image sprite;
} Spritesheet;

typedef Rect SpritesheetClip;

typedef struct {
    Vertex *vertices;
    int maxVertices, vertexCount;
    sg_bindings bind;
    int w, h;
} Spritebatch;

typedef struct {
    Spritebatch *batch;
} Renderable;

typedef struct {
    SpritesheetClip clip;
    Vec2 position;
} Sprite;

Spritesheet LoadSpritesheet(const char *path);
Spritesheet NewEmptySpritesheet(int w, int h);
void DestroySpritesheet(Spritesheet *sheet);

Spritebatch NewSpritebatch(Spritesheet *sheet, int maxVertices);
void DestroySpritebatch(Spritebatch *batch);
void SpritebatchBegin(Spritebatch *batch);
void SpritebatchRenderEx(Spritebatch *batch, int rw, int rh, Rect bounds, Rect clip, Vec4 color);
void SpritebatchRender(Spritesheet *batch, int x, int y, Rect clip);
void SpritebatchEnd(Spritebatch *batch);

#endif /* sprite_h */
