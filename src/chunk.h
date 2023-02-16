//
//  chunk.h
//  colony
//
//  Created by George Watson on 15/02/2023.
//

#ifndef chunk_h
#define chunk_h
#include "ecs.h"
#include "maths.h"
#include "renderer.h"

extern Entity EcsChunkComponent;

#define TILE_WIDTH 32
#define TILE_HEIGHT 16
#define HALF_TILE_WIDTH (TILE_WIDTH/2)
#define HALF_TILE_HEIGHT (TILE_HEIGHT/2)

#define CHUNK_WIDTH 512
#define CHUNK_HEIGHT CHUNK_WIDTH
#define CHUNK_SIZE (CHUNK_WIDTH * CHUNK_HEIGHT)
#define CHUNK_AT(X, Y) ((Y) * CHUNK_WIDTH + (X))

#define CHUNK_REAL_WIDTH (CHUNK_WIDTH * TILE_WIDTH)
#define CHUNK_REAL_HEIGHT (CHUNK_HEIGHT * HALF_TILE_HEIGHT)

#define MAX_CHUNKS 9

typedef enum {
    CHUNK_FREE,
    CHUNK_RESERVED,
    CHUNK_VISIBLE
} ChunkState;

typedef struct {
    int x, y;
    int tiles[CHUNK_SIZE];
} Chunk;

Vec2i CalcChunk(Vec2 position);
ChunkState CalcChunkState(int x, int y, Vec2 cameraPosition, Vec2 cameraSize);
Entity AddChunk(World *world, int x, int y);
void RenderChunk(Chunk *chunk, Vec2 cameraPosition, Vec2 cameraSize, TextureBatch *batch);

#endif /* chunk_h */