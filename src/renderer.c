//
//  renderer.c
//  colony
//
//  Created by George Watson on 09/02/2023.
//

#include "renderer.h"

static Texture LoadTexture(const char *path) {
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

static void DestroyTexture(Texture *texture) {
    if (texture && sg_query_image_state(texture->ref) != SG_RESOURCESTATE_INVALID) {
        sg_destroy_image(texture->ref);
        memset(texture, 0, sizeof(Texture));
    }
}

static TextureBatch NewTextureBatch(const char *path, int maxVertices) {
    Texture texture = LoadTexture(path);
    return (TextureBatch) {
        .vertices = malloc(6 * maxVertices * sizeof(Vertex)),
        .maxVertices = maxVertices,
        .vertexCount = 0,
        .binding = {
            .vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc) {
                .usage = SG_USAGE_STREAM,
                .size = 6 * maxVertices * sizeof(Vertex)
            }),
            .fs_images[SLOT_sprite] = texture.ref
        },
        .texture = texture
    };
}

static void ResetTextureBatch(TextureBatch *batch) {
    memset(batch->vertices, 0, 6 * batch->maxVertices * sizeof(Vertex));
    batch->vertexCount = 0;
}

static void CommitTextureBatch(TextureBatch *batch) {
    if (!batch || !batch->vertexCount)
        return;
    sg_update_buffer(batch->binding.vertex_buffers[0], &(sg_range) {
        .ptr = batch->vertices,
        .size = 6 * batch->vertexCount * sizeof(Vertex)
    });
    sg_apply_bindings(&batch->binding);
    sg_draw(0, 6 * batch->vertexCount, 1);
}

static void DestroyTextureBatch(TextureBatch *batch) {
    if (batch && sg_query_buffer_state(batch->binding.vertex_buffers[0]) != SG_RESOURCESTATE_INVALID) {
        sg_destroy_buffer(batch->binding.vertex_buffers[0]);
        if (batch->vertices)
            free(batch->vertices);
        if (sg_query_image_state(batch->texture.ref) != SG_RESOURCESTATE_INVALID)
            DestroyTexture(&batch->texture);
    }
}

static uint64_t HashBatch(const void *item, uint64_t seed0, uint64_t seed1) {
    TextureBucket *bucket = (TextureBucket*)item;
    return hashmap_sip(bucket->name, strlen(bucket->name), seed0, seed1);
}

static int CompareBatch(const void *a, const void *b, void*_) {
    TextureBucket *bucketA = (TextureBucket*)a;
    TextureBucket *bucketB = (TextureBucket*)b;
    return strcmp(bucketA->name, bucketB->name);
}

static void FreeBatch(void *item) {
    TextureBucket *bucket = (TextureBucket*)item;
    DestroyTextureBatch(&bucket->batch);
}

RenderPass NewRenderPass(int w, int h) {
    return (RenderPass) {
        .size = {w,h},
        .pass_action = (sg_pass_action) {
            .colors[0] = { .action=SG_ACTION_CLEAR, .value={0.f, 0.f, 0.f, 1.f} }
        },
        .pipeline = sg_make_pipeline(&(sg_pipeline_desc) {
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
        .textures = hashmap_new(sizeof(TextureBucket), 0, 0, 0, HashBatch, CompareBatch, FreeBatch, NULL)
    };
}

bool ResetBatches(const void *item, void*_) {
    TextureBucket *bucket = (TextureBucket*)item;
    ResetTextureBatch(&bucket->batch);
    return true;
}

bool CommitBatches(const void *item, void*_) {
    TextureBucket *bucket = (TextureBucket*)item;
    CommitTextureBatch(&bucket->batch);
    return true;
}

void RenderPassBegin(RenderPass *pass) {
    sg_begin_default_pass(&pass->pass_action, pass->size.x == 0.f ? sapp_width() : pass->size.x, pass->size.y == 0.f ? sapp_width() : pass->size.y);
    sg_apply_pipeline(pass->pipeline);
    hashmap_scan(pass->textures, ResetBatches, NULL);
}

void RenderPassEnd(RenderPass *pass) {
    hashmap_scan(pass->textures, CommitBatches, NULL);
    sg_end_pass();
}

void RenderPassNewBatch(RenderPass *pass, const char *path, int maxVertices) {
    TextureBucket search = {.name=path};
    TextureBucket *found = NULL;
    if ((found = hashmap_get(pass->textures, (void*)&search)))
        return;
    search.batch = NewTextureBatch(path, maxVertices);
    hashmap_set(pass->textures, (void*)&search);
}

TextureBatch* RenderPassGetBatch(RenderPass *pass, const char *path) {
    TextureBucket search = {.name=path};
    TextureBucket *found = NULL;
    if (!(found = hashmap_get(pass->textures, (void*)&search)))
        return NULL;
    return &found->batch;
}

void TextureBatchRender(TextureBatch *batch, Vec2 position, Vec2 size, Vec2 scale, Vec2 viewportSize, float rotation, Rect clip) {
    Vec2 quad[4] = {
        {position.x, position.y + size.y}, // bottom left
        {position.x + size.x, position.y + size.y}, // bottom right
        {position.x + size.x, position.y }, // top right
        {position.x, position.y }, // top left
    };
    float vw =  2.f / viewportSize.x;
    float vh = -2.f / viewportSize.y;
    for (int j = 0; j < 4; j++)
        quad[j] = (Vec2) {
            vw * quad[j].x + -1.f,
            vh * quad[j].y +  1.f
        };
    
    float iw = 1.f/batch->texture.size.x, ih = 1.f/batch->texture.size.y;
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

void DestroyRenderPass(RenderPass *pass) {
    if (sg_query_pipeline_state(pass->pipeline) != SG_RESOURCESTATE_INVALID) {
        sg_destroy_pipeline(pass->pipeline);
        hashmap_free(pass->textures);
        memset(pass, 0, sizeof(RenderPass));
    }
}
