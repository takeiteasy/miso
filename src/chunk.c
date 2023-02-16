//
//  chunk.c
//  colony
//
//  Created by George Watson on 15/02/2023.
//

#include "chunk.h"

Vec2i CalcChunk(Vec2 position) {
    return (Vec2i) {
        (int)floorf(position.x / (float)CHUNK_REAL_WIDTH),
        (int)floorf(position.y / (float)CHUNK_REAL_HEIGHT)
    };
}

ChunkState CalcChunkState(int x, int y, Vec2 cameraPosition, Vec2 cameraSize) {
    Rect cameraRect = {
        .pos = cameraPosition - (cameraSize / 2),
        .size = cameraSize
    };
    Rect reservedRect = {
        .pos = cameraPosition - cameraSize,
        .size = cameraSize * 2.f
    };
    static Vec2 chunkSize = {
        CHUNK_REAL_WIDTH,
        CHUNK_REAL_HEIGHT
    };
    Rect chunkRect = {
        .pos = {
            x * chunkSize.x,
            y * chunkSize.y
        },
        .size = chunkSize
    };
    return DoRectsCollide(cameraRect, chunkRect) ? CHUNK_VISIBLE : DoRectsCollide(reservedRect, chunkRect) ? CHUNK_RESERVED : CHUNK_FREE;
}

void AddChunk(World *world, int x, int y) {
    Entity e = EcsNewEntity(world);
    EcsAttach(world, e, EcsChunkComponent);
    Chunk *chunk = EcsGet(world, e, EcsChunkComponent);
    chunk->x = x;
    chunk->y = y;
}

static int IntToIndex(int i) {
    return abs(i * 2) - (i > 0 ? 1 : 0);
}

static int ChunkIndex(int x, int y) {
    int _x = IntToIndex(x), _y = IntToIndex(y);
    return _x >= _y ? _x * _x + _x + _y : _x + _y * _y;
}

void RenderChunk(Chunk *chunk, Vec2 cameraPosition, Vec2 cameraSize, TextureBatch *batch) {
    Vec2 chunkPosition = (Vec2){chunk->x * CHUNK_WIDTH, chunk->y * CHUNK_HEIGHT};
    Vec2 offset = chunkPosition * (Vec2){TILE_WIDTH,HALF_TILE_HEIGHT} + (-cameraPosition + cameraSize / 2);
    Rect viewportBounds = {{0, 0}, cameraSize};
    
    for (int x = 0; x < CHUNK_WIDTH; x++)
        for (int y = 0; y < CHUNK_HEIGHT; y++) {
            float px = offset.x + ((float)x * (float)TILE_WIDTH) + (y % 2 ? HALF_TILE_WIDTH : 0);
            float py = offset.y + ((float)y * (float)TILE_HEIGHT) - (y * HALF_TILE_HEIGHT);
            Rect bounds = {{px, py}, {TILE_WIDTH, TILE_HEIGHT}};
            if (!DoRectsCollide(viewportBounds, bounds))
                continue;
            TextureBatchRender(batch, (Vec2){px,py}, (Vec2){TILE_WIDTH,TILE_HEIGHT}, (Vec2){1.f,1.f}, cameraSize, 0.f, (Rect){{ChunkIndex(chunk->x, chunk->y) % 4 * 32, 0}, {TILE_WIDTH, TILE_HEIGHT}});
        }
}
