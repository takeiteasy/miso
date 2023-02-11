//
//  sprite.c
//  colony
//
//  Created by George Watson on 09/02/2023.
//

#include "sprite.h"

Spritesheet LoadSpritesheet(const char *path) {
    Bitmap image = LoadBitmap(path);
    Spritesheet result = {
        .w = image.w,
        .h = image.h,
        .sprite = sg_make_image(&(sg_image_desc) {
            .width = image.w,
            .height = image.h,
            .data.subimage[0][0] = (sg_range){.ptr=image.buf, .size=(image.w * image.h * sizeof(int))}
        })
    };
    DestroyBitmap(&image);
    return result;
}

Spritesheet NewEmptySpritesheet(int w, int h) {
    return (Spritesheet) {
        .w = w,
        .h = h,
        .sprite = sg_make_image(&(sg_image_desc) {
            .width = w,
            .height = h,
            .usage = SG_USAGE_STREAM
        })
    };
}

void DestroySpritesheet(Spritesheet *sheet) {
    if (sheet && sg_query_image_state(sheet->sprite) != SG_RESOURCESTATE_INVALID) {
        sg_destroy_image(sheet->sprite);
        memset(sheet, 0, sizeof(Spritesheet));
    }
}

Spritebatch NewSpritebatch(Spritesheet *sheet, int maxVertices) {
    return (Spritebatch) {
        .maxVertices = maxVertices,
        .vertexCount = 0,
        .w = sheet->w,
        .h = sheet->h,
        .vertices = malloc(6 * maxVertices * sizeof(Vertex)),
        .bind = (sg_bindings) {
            .vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc) {
                .usage = SG_USAGE_STREAM,
                .size = 6 * maxVertices * sizeof(Vertex)
            }),
            .fs_images[SLOT_sprite] = sheet->sprite
        }
    };
}

void DestroySpritebatch(Spritebatch *batch) {
    if (batch && sg_query_buffer_state(batch->bind.vertex_buffers[0]) != SG_RESOURCESTATE_INVALID) {
        sg_destroy_buffer(batch->bind.vertex_buffers[0]);
        if (batch->vertices)
            free(batch->vertices);
    }
}

void SpritebatchBegin(Spritebatch *batch) {
    memset(batch->vertices, 0, 6 * batch->maxVertices * sizeof(Vertex));
    batch->vertexCount = 0;
}

void SpritebatchRenderEx(Spritebatch *batch, int rw, int rh, Rect bounds, Rect clip, Vec4 color) {
    Vec2 quad[4] = {
        {bounds.pos.x, bounds.pos.y + bounds.size.y}, // bottom left
        {bounds.pos.x + bounds.size.x, bounds.pos.y + bounds.size.y}, // bottom right
        {bounds.pos.x + bounds.size.x, bounds.pos.y }, // top right
        {bounds.pos.x, bounds.pos.y }, // top left
    };
    float vw =  2.f / (float)rw;
    float vh = -2.f / (float)rh;
    for (int j = 0; j < 4; j++)
        quad[j] = (Vec2) {
            vw * quad[j].x + -1.f,
            vh * quad[j].y +  1.f
        };
    
    Vertex *v = &batch->vertices[batch->vertexCount++ * 6];
    v[0].position = quad[0];
    v[1].position = quad[1];
    v[2].position = quad[2];
    v[3].position = quad[3];
    v[4].position = quad[0];
    v[5].position = quad[2];
    
    for (int i = 0; i < 6; i++)
        v[i].color = color;
    
    float iw = 1.f/batch->w, ih = 1.f/(float)batch->h;
    float tl = clip.pos.x*iw;
    float tt = clip.pos.y*ih;
    float tr = (clip.pos.x + clip.size.x)*iw;
    float tb = (clip.pos.y + clip.size.y)*ih;
    Vec2 vtexquad[4] = {
        {tl, tb}, // bottom left
        {tr, tb}, // bottom right
        {tr, tt}, // top right
        {tl, tt}, // top left
    };
    
    v[0].texcoord = vtexquad[0];
    v[1].texcoord = vtexquad[1];
    v[2].texcoord = vtexquad[2];
    v[3].texcoord = vtexquad[3];
    v[4].texcoord = vtexquad[0];
    v[5].texcoord = vtexquad[2];
}

void SpritebatchRender(Spritesheet *batch, int x, int y, Rect clip) {
    //! TODO: SpritebatchRender
}

void SpritebatchEnd(Spritebatch *batch) {
    if (!batch || !batch->vertexCount)
        return;
    sg_update_buffer(batch->bind.vertex_buffers[0], &(sg_range) {
        .ptr = batch->vertices,
        .size = 6 * batch->vertexCount * sizeof(Vertex)
    });
    sg_apply_bindings(&batch->bind);
    sg_draw(0, 6 * batch->vertexCount, 1);
}
