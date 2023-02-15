//
//  renderer.c
//  colony
//
//  Created by George Watson on 09/02/2023.
//

#include "renderer.h"

Texture LoadTexture(const char *path) {
    Bitmap image = LoadBitmap(path);
    Texture result = {
        .size = {image.w, image.h},
        .ref = sg_make_image(&(sg_image_desc) {
            .width = image.w,
            .height = image.h,
            .data.subimage[0][0] = (sg_range){.ptr=image.buf, .size=image.w * image.h * sizeof(int)}
        })
    };
    DestroyBitmap(&image);
    return result;
}

Texture NewTexture(int w, int h) {
    return (Texture) {
        .size = {w, h},
        .ref = sg_make_image(&(sg_image_desc) {
            .width = w,
            .height = h,
            .usage = SG_USAGE_STREAM
        })
    };
}

void DestroyTexture(Texture *texture) {
    if (texture && sg_query_image_state(texture->ref) != SG_RESOURCESTATE_INVALID) {
        sg_destroy_image(texture->ref);
        memset(texture, 0, sizeof(Texture));
    }
}

TextureBatch NewTextureBatch(Texture *texture, int maxVertices) {
    return (TextureBatch) {
        .vertices = malloc(6 * maxVertices * sizeof(Vertex)),
        .maxVertices = maxVertices,
        .vertexCount = 0,
        .bind = {
            .vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc) {
                .usage = SG_USAGE_STREAM,
                .size = 6 * maxVertices * sizeof(Vertex)
            }),
            .fs_images[SLOT_sprite] = texture->ref
        },
        .size = texture->size
    };
}

void ResetTextureBatch(TextureBatch *batch) {
    memset(batch->vertices, 0, 6 * batch->maxVertices * sizeof(Vertex));
    batch->vertexCount = 0;
}

void TextureBatchRender(TextureBatch *batch, Vec2 position, Vec2 size, Vec2 scale, float rotation, Rect clip) {
    Vec2 quad[4] = {
        {position.x, position.y + size.y}, // bottom left
        {position.x + size.x, position.y + size.y}, // bottom right
        {position.x + size.x, position.y }, // top right
        {position.x, position.y }, // top left
    };
    float vw =  2.f / (float)sapp_width();
    float vh = -2.f / (float)sapp_height();
    for (int j = 0; j < 4; j++)
        quad[j] = (Vec2) {
            vw * quad[j].x + -1.f,
            vh * quad[j].y +  1.f
        };
    
    float iw = 1.f/batch->size.x, ih = 1.f/batch->size.y;
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
    
    static int indices[6] = {
        0, 1, 2,
        3, 0, 2
    };
    
    Vertex result[6];
    for (int i = 0; i < 6; i++)
        result[i] = (Vertex) {
            .position = quad[indices[i]],
            .texcoord = vtexquad[indices[i]],
            .color = NoColor
        };
    memcpy(&batch->vertices[batch->vertexCount++ * 6], result, 6 * sizeof(Vertex));
}

void CommitTextureBatch(TextureBatch *batch) {
    if (!batch || !batch->vertexCount)
        return;
    sg_update_buffer(batch->bind.vertex_buffers[0], &(sg_range) {
        .ptr = batch->vertices,
        .size = 6 * batch->vertexCount * sizeof(Vertex)
    });
    sg_apply_bindings(&batch->bind);
    sg_draw(0, 6 * batch->vertexCount, 1);
}

void DestroyTextureBatch(TextureBatch *batch) {
    if (batch && sg_query_buffer_state(batch->bind.vertex_buffers[0]) != SG_RESOURCESTATE_INVALID) {
        sg_destroy_buffer(batch->bind.vertex_buffers[0]);
        if (batch->vertices)
            free(batch->vertices);
    }
}

RenderPass NewRenderPass(int w, int h, RenderPassCb cb) {
    return (RenderPass) {
        .size = {w,h},
        .pass_action = (sg_pass_action) {
            .colors[0] = { .action=SG_ACTION_CLEAR, .value={0.f, 0.f, 0.f, 1.f} }
        },
        .pip = sg_make_pipeline(&(sg_pipeline_desc) {
            .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
            .shader = sg_make_shader(sprite_program_shader_desc(sg_query_backend())),
            .layout = {
                .buffers[0].stride = sizeof(Vertex),
                .attrs = {
                    [ATTR_sprite_vs_position].format=SG_VERTEXFORMAT_FLOAT2,
                    [ATTR_sprite_vs_texcoord].format=SG_VERTEXFORMAT_FLOAT2,
                    [ATTR_sprite_vs_color].format=SG_VERTEXFORMAT_FLOAT4
                }
            },
            .colors[0] = {
                .blend = {
                    .enabled = true,
                    .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
                    .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                    .op_rgb = SG_BLENDOP_ADD,
                    .src_factor_alpha = SG_BLENDFACTOR_ONE,
                    .dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                    .op_alpha = SG_BLENDOP_ADD
                }
            }
        }),
        .cb = cb
    };
}

void RunRenderPass(RenderPass *pass) {
    sg_begin_default_pass(&pass->pass_action, pass->size.x, pass->size.y);
    sg_apply_pipeline(pass->pip);
    pass->cb(pass);
    sg_end_pass();
}

void DestroyRenderPass(RenderPass *pass) {
    if (sg_query_pipeline_state(pass->pip) != SG_RESOURCESTATE_INVALID) {
        sg_destroy_pipeline(pass->pip);
        memset(pass, 0, sizeof(RenderPass));
    }
}
