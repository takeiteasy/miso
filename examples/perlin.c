//
//  perlin.c
//  miso
//
//  Created by George Watson on 19/08/2023.
//

#include "miso.h"
#define SOKOL_IMPL
#include "sokol_app.h"
#include "sokol_glue.h"

static const float grad3[][3] = {
    { 1, 1, 0 }, { -1, 1, 0 }, { 1, -1, 0 }, { -1, -1, 0 },
    { 1, 0, 1 }, { -1, 0, 1 }, { 1, 0, -1 }, { -1, 0, -1 },
    { 0, 1, 1 }, { 0, -1, 1 }, { 0, 1, -1 }, { 0, -1, -1 }
};

static const unsigned int perm[] = {
    182, 232, 51, 15, 55, 119, 7, 107, 230, 227, 6, 34, 216, 61, 183, 36,
    40, 134, 74, 45, 157, 78, 81, 114, 145, 9, 209, 189, 147, 58, 126, 0,
    240, 169, 228, 235, 67, 198, 72, 64, 88, 98, 129, 194, 99, 71, 30, 127,
    18, 150, 155, 179, 132, 62, 116, 200, 251, 178, 32, 140, 130, 139, 250, 26,
    151, 203, 106, 123, 53, 255, 75, 254, 86, 234, 223, 19, 199, 244, 241, 1,
    172, 70, 24, 97, 196, 10, 90, 246, 252, 68, 84, 161, 236, 205, 80, 91,
    233, 225, 164, 217, 239, 220, 20, 46, 204, 35, 31, 175, 154, 17, 133, 117,
    73, 224, 125, 65, 77, 173, 3, 2, 242, 221, 120, 218, 56, 190, 166, 11,
    138, 208, 231, 50, 135, 109, 213, 187, 152, 201, 47, 168, 185, 186, 167, 165,
    102, 153, 156, 49, 202, 69, 195, 92, 21, 229, 63, 104, 197, 136, 148, 94,
    171, 93, 59, 149, 23, 144, 160, 57, 76, 141, 96, 158, 163, 219, 237, 113,
    206, 181, 112, 111, 191, 137, 207, 215, 13, 83, 238, 249, 100, 131, 118, 243,
    162, 248, 43, 66, 226, 27, 211, 95, 214, 105, 108, 101, 170, 128, 210, 87,
    38, 44, 174, 188, 176, 39, 14, 143, 159, 16, 124, 222, 33, 247, 37, 245,
    8, 4, 22, 82, 110, 180, 184, 12, 25, 5, 193, 41, 85, 177, 192, 253,
    79, 29, 115, 103, 142, 146, 52, 48, 89, 54, 121, 212, 122, 60, 28, 42,
    
    182, 232, 51, 15, 55, 119, 7, 107, 230, 227, 6, 34, 216, 61, 183, 36,
    40, 134, 74, 45, 157, 78, 81, 114, 145, 9, 209, 189, 147, 58, 126, 0,
    240, 169, 228, 235, 67, 198, 72, 64, 88, 98, 129, 194, 99, 71, 30, 127,
    18, 150, 155, 179, 132, 62, 116, 200, 251, 178, 32, 140, 130, 139, 250, 26,
    151, 203, 106, 123, 53, 255, 75, 254, 86, 234, 223, 19, 199, 244, 241, 1,
    172, 70, 24, 97, 196, 10, 90, 246, 252, 68, 84, 161, 236, 205, 80, 91,
    233, 225, 164, 217, 239, 220, 20, 46, 204, 35, 31, 175, 154, 17, 133, 117,
    73, 224, 125, 65, 77, 173, 3, 2, 242, 221, 120, 218, 56, 190, 166, 11,
    138, 208, 231, 50, 135, 109, 213, 187, 152, 201, 47, 168, 185, 186, 167, 165,
    102, 153, 156, 49, 202, 69, 195, 92, 21, 229, 63, 104, 197, 136, 148, 94,
    171, 93, 59, 149, 23, 144, 160, 57, 76, 141, 96, 158, 163, 219, 237, 113,
    206, 181, 112, 111, 191, 137, 207, 215, 13, 83, 238, 249, 100, 131, 118, 243,
    162, 248, 43, 66, 226, 27, 211, 95, 214, 105, 108, 101, 170, 128, 210, 87,
    38, 44, 174, 188, 176, 39, 14, 143, 159, 16, 124, 222, 33, 247, 37, 245,
    8, 4, 22, 82, 110, 180, 184, 12, 25, 5, 193, 41, 85, 177, 192, 253,
    79, 29, 115, 103, 142, 146, 52, 48, 89, 54, 121, 212, 122, 60, 28, 42
};

static float dot3(const float a[], float x, float y, float z) {
    return a[0]*x + a[1]*y + a[2]*z;
}

static float lerp(float a, float b, float t) {
    return (1 - t) * a + t * b;
}

static float fade(float t) {
    return t * t * t * (t * (t * 6 - 15) + 10);
}

#define FASTFLOOR(x)  (((x) >= 0) ? (int)(x) : (int)(x)-1)

float Perlin(float x, float y, float z) {
    /* Find grid points */
    int gx = FASTFLOOR(x);
    int gy = FASTFLOOR(y);
    int gz = FASTFLOOR(z);
    
    /* Relative coords within grid cell */
    float rx = x - gx;
    float ry = y - gy;
    float rz = z - gz;
    
    /* Wrap cell coords */
    gx = gx & 255;
    gy = gy & 255;
    gz = gz & 255;
    
    /* Calculate gradient indices */
    unsigned int gi[8];
    for (int i = 0; i < 8; i++)
        gi[i] = perm[gx+((i>>2)&1)+perm[gy+((i>>1)&1)+perm[gz+(i&1)]]] % 12;
    
    /* Noise contribution from each corner */
    float n[8];
    for (int i = 0; i < 8; i++)
        n[i] = dot3(grad3[gi[i]], rx - ((i>>2)&1), ry - ((i>>1)&1), rz - (i&1));
    
    /* Fade curves */
    float u = fade(rx);
    float v = fade(ry);
    float w = fade(rz);
    
    /* Interpolate */
    float nx[4];
    for (int i = 0; i < 4; i++)
        nx[i] = lerp(n[i], n[4+i], u);
    
    float nxy[2];
    for (int i = 0; i < 2; i++)
        nxy[i] = lerp(nx[i], nx[2+i], v);
    
    return lerp(nxy[0], nxy[1], w);
}

static float Remap(float value, float low1, float high1, float low2, float high2) {
    return low2 + (value - low1) * (high2 - low2) / (high1 - low1);
}

unsigned char* PerlinFBM(int w, int h, float xoff, float yoff, float z, float scale, float lacunarity, float gain, int octaves) {
    float min = FLT_MAX, max = FLT_MIN;
    float *grid = malloc(w * h * sizeof(float));
    for (int x = 0; x < w; ++x)
        for (int y = 0; y < h; ++y) {
            float freq = 2.f,
                  amp  = 1.f,
                  tot  = 0.f,
                  sum  = 0.f;
            for (int i = 0; i < octaves; ++i) {
                sum  += Perlin(((xoff + x) / scale) * freq, ((yoff + y) / scale) * freq, z) * amp;
                tot  += amp;
                freq *= lacunarity;
                amp  *= gain;
            }
            grid[y * w + x] = sum = (sum / tot);
            if (sum < min)
                min = sum;
            if (sum > max)
                max = sum;
        }
    
    unsigned char *result = malloc(w * h * sizeof(unsigned char));
    for (int x = 0; x < w; x++)
        for (int y = 0; y < h; y++) {
            float height = 255.f - (255.f * Remap(grid[y * w + x], min, max, 0, 1.f));
            float grad = sqrtf(powf(w / 2 - x, 2.f) + powf(h / 2 - y, 2.f));
            float final = height - grad;
            result[y * w + x] = (unsigned char)final;
        }
    free(grid);
    return result;
}

static struct {
    MisoTexture *texture;
    MisoChunk *map;
    MisoCamera camera;
    bool mouseDown;
} state;

#define MAP_SIZE 256
#define TILE_WIDTH 32.f
#define TILE_HEIGHT 16.f

static void init(void) {
    sg_desc desc = {
        .context = sapp_sgcontext()
    };
    sg_setup(&desc);
    
    OrderMiso();
    
    state.camera.position = (MisoVec2) {
        MAP_SIZE * TILE_WIDTH / 2.f,
        MAP_SIZE * (TILE_HEIGHT / 2.f) / 2.f
    };
    state.camera.zoom = 1.f;
    state.texture = MisoLoadTextureFromFile("assets/test.png");
    state.map = MisoEmptyChunk(state.texture, MAP_SIZE, MAP_SIZE, TILE_WIDTH, TILE_HEIGHT);
    unsigned char *noise = PerlinFBM(MAP_SIZE, MAP_SIZE, 0.f, 0.f, 0.f, 200.f, 2.f, .5f, 8);
    for (int x = 0; x < MAP_SIZE; x++)
        for (int y = 0; y < MAP_SIZE; y++) {
            float v = (int)Remap((float)noise[y * MAP_SIZE + x], 0.f, 255.f, 0.f, 5.f);
            MisoChunkSet(state.map, x, y, v);
        }
    free(noise);
}

static void frame(void) {
    OrderUp(sapp_width(), sapp_height());
    MisoDrawChunk(state.map, &state.camera);
    FinishMiso();
    sg_commit();
}

static void event(const sapp_event *e) {
    switch (e->type) {
        case SAPP_EVENTTYPE_MOUSE_DOWN:
            if (e->mouse_button == SAPP_MOUSEBUTTON_LEFT)
                state.mouseDown = true;
            break;
        case SAPP_EVENTTYPE_MOUSE_UP:
            if (e->mouse_button == SAPP_MOUSEBUTTON_LEFT)
                state.mouseDown = false;
            break;
        case SAPP_EVENTTYPE_MOUSE_MOVE:
            if (state.mouseDown) {
                state.camera.position = (MisoVec2) {
                    state.camera.position.x - e->mouse_dx,
                    state.camera.position.y - e->mouse_dy,
                };
            }
            break;
        default:
            break;
    }
}

static void cleanup(void) {
    MisoDestroyTexture(state.texture);
    MisoDestroyChunk(state.map);
    CleanUpMiso();
    sg_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[]) {
    return (sapp_desc) {
        .width = 640,
        .height = 480,
        .window_title = "miso -- perlin.c",
        .init_cb = init,
        .frame_cb = frame,
        .event_cb = event,
        .cleanup_cb = cleanup
    };
}
