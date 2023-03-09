//
//  texture.c
//  colony
//
//  Created by George Watson on 09/02/2023.
//

#include "texture.h"

Texture LoadTexture(const char *path) {
    Bitmap image = LoadBitmap(path);
    Texture result = sg_make_image(&(sg_image_desc) {
        .width = image.w,
        .height = image.h,
        .wrap_u = SG_WRAP_REPEAT,
        .wrap_v = SG_WRAP_REPEAT,
        .min_filter = SG_FILTER_NEAREST_MIPMAP_NEAREST,
        .mag_filter = SG_FILTER_NEAREST_MIPMAP_NEAREST,
        .data.subimage[0][0] = (sg_range){.ptr=image.buf, .size=image.w * image.h * sizeof(int)}
    });
    DestroyBitmap(&image);
    return result;
}

Texture MutableTexture(int w, int h) {
    return sg_make_image(&(sg_image_desc) {
        .width = w,
        .height = h,
        .usage = SG_USAGE_STREAM
    });
}

void DestroyTexture(Texture texture) {
    if (sg_query_image_state(texture) == SG_RESOURCESTATE_VALID)
        sg_destroy_image(texture);
}

TextureBatch NewTextureBatch(Texture texture, int maxVertices) {
    sg_image_info info = sg_query_image_info(texture);
    return (TextureBatch) {
        .vertices = malloc(6 * maxVertices * sizeof(Vertex)),
        .maxVertices = maxVertices,
        .vertexCount = 0,
        .binding = {
            .fs_images[SLOT_sprite] = texture,
            .vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc) {
                .usage = SG_USAGE_STREAM,
                .size = 6 * maxVertices * sizeof(Vertex)
            })
        },
        .size = {info.width, info.height}
    };
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
        } * scale;
    
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
            .texcoord = vtexquad[indices[i]]
        };
    memcpy(&batch->vertices[batch->vertexCount++ * 6], result, 6 * sizeof(Vertex));
}

void CommitTextureBatch(TextureBatch *batch) {
    if (!batch || !batch->vertexCount)
        return;
    sg_update_buffer(batch->binding.vertex_buffers[0], &(sg_range) {
        .ptr = batch->vertices,
        .size = 6 * batch->vertexCount * sizeof(Vertex)
    });
    sg_apply_bindings(&batch->binding);
    sg_draw(0, 6 * batch->vertexCount, 1);
    memset(batch->vertices, 0, 6 * batch->maxVertices * sizeof(Vertex));
    batch->vertexCount = 0;
}

void DestroyTextureBatch(TextureBatch *batch) {
    if (batch && sg_query_buffer_state(batch->binding.vertex_buffers[0]) != SG_RESOURCESTATE_INVALID) {
        sg_destroy_buffer(batch->binding.vertex_buffers[0]);
        if (batch->vertices)
            free(batch->vertices);
    }
}

typedef struct {
    const char *name;
    Texture texture;
} TextureBucket;

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
    if (sg_query_image_state(bucket->texture) != SG_RESOURCESTATE_INVALID)
        sg_destroy_image(bucket->texture);
}

TextureManager NewTextureManager(void) {
    return (TextureManager) {
        .map = hashmap_new(sizeof(TextureBucket), 0, 0, 0, HashBatch, CompareBatch, FreeBatch, NULL)
    };
}

void TextureManagerAdd(TextureManager *manager, const char *path) {
    TextureBucket search = {.name=path};
    TextureBucket *found = NULL;
    if ((found = hashmap_get(manager->map, (void*)&search)))
        return;
    if ((sg_query_image_state((search.texture = LoadTexture(path)))) != SG_RESOURCESTATE_INVALID)
        hashmap_set(manager->map, (void*)&search);
}

void TextureManagerNew(TextureManager *manager, const char *name, int w, int h) {
    TextureBucket search = {.name=name};
    TextureBucket *found = NULL;
    if ((found = hashmap_get(manager->map, (void*)&search)))
        return;
    if ((sg_query_image_state((search.texture = MutableTexture(w, h)))) != SG_RESOURCESTATE_INVALID)
        hashmap_set(manager->map, (void*)&search);
}

Texture TextureManagerGet(TextureManager *manager, const char *path) {
    TextureBucket search = {.name=path};
    TextureBucket *found = NULL;
    if (!(found = hashmap_get(manager->map, (void*)&search)))
        return (sg_image){SG_INVALID_ID};
    return found->texture;
}

void DestroyTextureManager(TextureManager *manager) {
    if (manager && manager->map)
        hashmap_free(manager->map);
}
