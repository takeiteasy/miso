//
//  miso.c
//  miso
//
//  Created by George Watson on 17/08/2023.
//


#define SOKOL_IMPL
#define SOKOL_NO_ENTRY
#define HASHMAP_IMPL
#define MJSON_IMPLEMENTATION
#define JIM_IMPLEMENTATION
#include "miso.h"
#include "../assets/sprite.glsl.h"
#include "../assets/framebuffer.glsl.h"

static struct {
    const char *configPath;
    void *userdata;
    bool running;
    bool mouseHidden;
    bool mouseLocked;
    sapp_desc desc;
    void (*init_cb)(void);
    void (*frame_cb)(void);
    void (*event_cb)(const sapp_event*);
    void (*cleanup_cb)(void);
    
    struct {
        bool button_down[SAPP_MAX_KEYCODES];
        bool button_clicked[SAPP_MAX_KEYCODES];
        bool mouse_down[SAPP_MAX_MOUSEBUTTONS];
        bool mouse_clicked[SAPP_MAX_MOUSEBUTTONS];
        Vector2 mouse_pos, last_mouse_pos;
        Vector2 mouse_scroll_delta, mouse_delta;
    } input;
    
    sg_pass_action pass_action;
    sg_pass pass;
    sg_pipeline framebuffer_pip, offscreen_pip;
    sg_bindings bind;
    sg_image color, depth;
} state = {
    .pass_action = {
        .colors[0] = { .action=SG_ACTION_CLEAR, .value={0.f, 0.f, 0.f, 1.f} }
    }
};

int WindowWidth(void) {
    return state.running ? sapp_width() : -1;
}

int WindowHeight(void) {
    return state.running ? sapp_height() : -1;
}

int IsWindowFullscreen(void) {
    return state.running ? sapp_is_fullscreen() : state.desc.fullscreen;
}

void ToggleFullscreen(void) {
    if (!state.running)
        state.desc.fullscreen = !state.desc.fullscreen;
    else
        sapp_toggle_fullscreen();
}

int IsCursorVisible(void) {
    return state.running ? sapp_mouse_shown() : state.mouseHidden;
}

void ToggleCursorVisible(void) {
    if (state.running)
        sapp_show_mouse(!state.mouseHidden);
    state.mouseHidden = !state.mouseHidden;
}

int IsCursorLocked(void) {
    return state.running? sapp_mouse_locked() : state.mouseLocked;
}

void ToggleCursorLock(void) {
    if (state.running)
        sapp_lock_mouse(!state.mouseLocked);
    state.mouseLocked = !state.mouseLocked;
}

void SetClearColor(Color color) {
    state.pass_action.colors[0].value.r = (float)color.r / 255.f;
    state.pass_action.colors[0].value.g = (float)color.g / 255.f;
    state.pass_action.colors[0].value.b = (float)color.b / 255.f;
    state.pass_action.colors[0].value.a = (float)color.a / 255.f;
}

bool IsKeyDown(sapp_keycode key) {
    return state.input.button_down[key];
}

bool IsKeyUp(sapp_keycode key) {
    return !state.input.button_down[key];
}

bool WasKeyPressed(sapp_keycode key) {
    return state.input.button_clicked[key];
}

bool IsButtonDown(sapp_mousebutton button) {
    return state.input.mouse_down[button];
}

bool IsButtonUp(sapp_mousebutton button) {
    return !state.input.mouse_down[button];
}

bool WasButtonPressed(sapp_mousebutton button) {
    return state.input.mouse_clicked[button];
}

bool WasMouseScrolled(void) {
    return state.input.mouse_scroll_delta.x != 0.f && state.input.mouse_scroll_delta.y != 0;
}

bool WasMouseMoved(void) {
    return state.input.mouse_delta.x != 0.f && state.input.mouse_delta.y != 0;;
}

Vector2 MousePosition(void) {
    return state.input.mouse_pos;
}

Vector2 LastMousePosition(void) {
    return state.input.last_mouse_pos;
}

Vector2 MouseScrollDelta(void) {
    return state.input.mouse_scroll_delta;
}

Vector2 MouseMoveDelta(void) {
    return state.input.mouse_delta;
}

bool DoesFileExist(const char *path) {
    return !access(path, F_OK);
}

bool DoesDirExist(const char *path) {
    struct stat sb;
    return stat(path, &sb) == 0 && S_ISDIR(sb.st_mode);
}

char* FormatString(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int size = vsnprintf(NULL, 0, fmt, args) + 1;
    char *buf = malloc(size);
    vsnprintf(buf, size, fmt, args);
    va_end(args);
    return buf;
}

char* LoadFile(const char *path, size_t *length) {
    char *result = NULL;
    size_t sz = -1;
    FILE *fh = fopen(path, "rb");
    if (!fh)
        goto BAIL;
    fseek(fh, 0, SEEK_END);
    sz = ftell(fh);
    fseek(fh, 0, SEEK_SET);

    result = malloc(sz * sizeof(char));
    fread(result, sz, 1, fh);
    fclose(fh);
    
BAIL:
    if (length)
        *length = sz;
    return result;
}

const char* JoinPath(const char *a, const char *b) {
    static char buffer[MAX_PATH];
    buffer[0] = '\0';
    strcat(buffer, a);
    const char *seperator = PATH_SEPERATOR;
    if (a[strlen(a)-1] != seperator[0] &&
        b[0] != seperator[0])
        strcat(buffer, seperator);
    strcat(buffer, b);
    return buffer;
}

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

Image* CreateImage(unsigned int w, unsigned int h) {
    Image *result = malloc(sizeof(Image));
    result->buf = malloc(w * h * sizeof(int));
    result->w = w;
    result->h = h;
    return result;
}

void DestroyImage(Image *img) {
    if (img) {
        if (img->buf)
            free(img->buf);
        free(img);
    }
}

void ImageSet(Image *img, int x, int y, Color col) {
    if (x >= 0 && y >= 0 && x < img->w && y < img->h)
        img->buf[y * img->w + x] = col.rgba;
}

Color ImageGet(Image *img, int x, int y) {
    return (Color) {
        .rgba = (x >= 0 && y >= 0 && x < img->w && y < img->h) ? img->buf[y * img->w + x] : 0
    };
}

typedef struct {
    const unsigned char *p, *end;
} PNG;

static unsigned get32(const unsigned char* v) {
    return (v[0] << 24) | (v[1] << 16) | (v[2] << 8) | v[3];
}

static const unsigned char* find(PNG* png, const char* chunk, unsigned minlen) {
    const unsigned char* start;
    while (png->p < png->end) {
        unsigned len = get32(png->p + 0);
        start = png->p;
        png->p += len + 12;
        if (memcmp(start + 4, chunk, 4) == 0 && len >= minlen && png->p <= png->end)
            return start + 8;
    }
    return NULL;
}

static unsigned char paeth(unsigned char a, unsigned char b, unsigned char c) {
    int p = a + b - c;
    int pa = abs(p - a), pb = abs(p - b), pc = abs(p - c);
    return (pa <= pb && pa <= pc) ? a : (pb <= pc) ? b : c;
}

static int rowBytes(int w, int bipp) {
    int rowBits = w * bipp;
    return rowBits / 8 + ((rowBits % 8) ? 1 : 0);
}

static int unfilter(int w, int h, int bipp, unsigned char* raw) {
    int len = rowBytes(w, bipp);
    int bpp = rowBytes(1, bipp);
    int x, y;
    unsigned char* first = (unsigned char*)malloc(len + 1);
    memset(first, 0, len + 1);
    unsigned char* prev = first;
    for (y = 0; y < h; y++, prev = raw, raw += len) {
#define LOOP(A, B)            \
    for (x = 0; x < bpp; x++) \
        raw[x] += A;          \
    for (; x < len; x++)      \
        raw[x] += B;          \
    break
        switch (*raw++) {
            case 0:
                break;
            case 1:
                LOOP(0, raw[x - bpp]);
            case 2:
                LOOP(prev[x], prev[x]);
            case 3:
                LOOP(prev[x] / 2, (raw[x - bpp] + prev[x]) / 2);
            case 4:
                LOOP(prev[x], paeth(raw[x - bpp], prev[x], prev[x - bpp]));
            default:
                return 0;
        }
#undef LOOP
    }
    free(first);
    return 1;
}

static int RGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    return ((unsigned int)a << 24) | ((unsigned int)b << 16) | ((unsigned int)g << 8) | r;
}

#define RGBA1(C, A) RGBA((C), (C), (C), (A))
#define RGB(R, G, B) RGBA((R), (G), (B), 255)
#define RGB1(C) RGB((C), (C), (C))

static void convert(int bypp, int w, int h, const unsigned char* src, int* dest, const unsigned char* trns) {
    int x, y;
    for (y = 0; y < h; y++) {
        src++;  // skip filter byte
        for (x = 0; x < w; x++, src += bypp) {
            switch (bypp) {
                case 1: {
                    unsigned char c = src[0];
                    if (trns && c == *trns) {
                        *dest++ = RGBA1(c, 0);
                        break;
                    } else {
                        *dest++ = RGB1(c);
                        break;
                    }
                }
                case 2:
                    *dest++ = RGBA(src[0], src[0], src[0], src[1]);
                    break;
                case 3: {
                    unsigned char r = src[0];
                    unsigned char g = src[1];
                    unsigned char b = src[2];
                    if (trns && trns[1] == r && trns[3] == g && trns[5] == b) {
                        *dest++ = RGBA(r, g, b, 0);
                        break;
                    } else {
                        *dest++ = RGB(r, g, b);
                        break;
                    }
                }
                case 4:
                    *dest++ = RGBA(src[0], src[1], src[2], src[3]);
                    break;
            }
        }
    }
}

static void depalette(int w, int h, unsigned char* src, int* dest, int bipp, const unsigned char* plte, const unsigned char* trns, int trnsSize) {
    int x, y, c;
    unsigned char alpha;
    int mask, len;

    switch (bipp) {
        case 4:
            mask = 15;
            len = 1;
            break;
        case 2:
            mask = 3;
            len = 3;
            break;
        case 1:
            mask = 1;
            len = 7;
    }

    for (y = 0; y < h; y++) {
        src++;  // skip filter byte
        for (x = 0; x < w; x++) {
            if (bipp == 8) {
                c = *src++;
            } else {
                int pos = x & len;
                c = (src[0] >> ((len - pos) * bipp)) & mask;
                if (pos == len) {
                    src++;
                }
            }
            alpha = 255;
            if (c < trnsSize) {
                alpha = trns[c];
            }
            *dest++ = RGBA(plte[c * 3 + 0], plte[c * 3 + 1], plte[c * 3 + 2], alpha);
        }
    }
}

static int outsize(Image* bmp, int bipp) {
    return (rowBytes(bmp->w, bipp) + 1) * bmp->h;
}

#define PNG_FAIL()      \
    {                   \
        errno = EINVAL; \
        goto err;       \
    }
#define PNG_CHECK(X) \
    if (!(X))        \
        PNG_FAIL()

typedef struct {
    unsigned bits, count;
    const unsigned char *in, *inend;
    unsigned char *out, *outend;
    jmp_buf jmp;
    unsigned litcodes[288], distcodes[32], lencodes[19];
    int tlit, tdist, tlen;
} State;

#define INFLATE_FAIL() longjmp(s->jmp, 1)
#define INFLATE_CHECK(X) \
    if (!(X))            \
        INFLATE_FAIL()

// Built-in DEFLATE standard tables.
static char order[] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };
static char lenBits[29 + 2] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
                                3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0, 0, 0 };
static int lenBase[29 + 2] = { 3,  4,  5,  6,  7,  8,  9,  10,  11,  13,  15,  17,  19,  23, 27, 31,
                               35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0,  0 };
static char distBits[30 + 2] = { 0, 0, 0, 0, 1, 1, 2,  2,  3,  3,  4,  4,  5,  5,  6, 6,
                                 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 0, 0 };
static int distBase[30 + 2] = {
    1,   2,   3,   4,   5,   7,    9,    13,   17,   25,   33,   49,   65,    97,    129,
    193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577
};

static const unsigned char reverseTable[256] = {
#define R2(n) n, n + 128, n + 64, n + 192
#define R4(n) R2(n), R2(n + 32), R2(n + 16), R2(n + 48)
#define R6(n) R4(n), R4(n + 8), R4(n + 4), R4(n + 12)
    R6(0), R6(2), R6(1), R6(3)
};

static unsigned rev16(unsigned n) {
    return (reverseTable[n & 0xff] << 8) | reverseTable[(n >> 8) & 0xff];
}

static int bits(State* s, int n) {
    int v = s->bits & ((1 << n) - 1);
    s->bits >>= n;
    s->count -= n;
    while (s->count < 16) {
        INFLATE_CHECK(s->in != s->inend);
        s->bits |= (*s->in++) << s->count;
        s->count += 8;
    }
    return v;
}

static unsigned char* emit(State* s, int len) {
    s->out += len;
    INFLATE_CHECK(s->out <= s->outend);
    return s->out - len;
}

static void copy(State* s, const unsigned char* src, int len) {
    unsigned char* dest = emit(s, len);
    while (len--)
        *dest++ = *src++;
}

static int build(State* s, unsigned* tree, unsigned char* lens, int symcount) {
    int n, codes[16], first[16], counts[16] = { 0 };

    // Frequency count.
    for (n = 0; n < symcount; n++)
        counts[lens[n]]++;

    // Distribute codes.
    counts[0] = codes[0] = first[0] = 0;
    for (n = 1; n <= 15; n++) {
        codes[n] = (codes[n - 1] + counts[n - 1]) << 1;
        first[n] = first[n - 1] + counts[n - 1];
    }
    INFLATE_CHECK(first[15] + counts[15] <= symcount);

    // Insert keys into the tree for each symbol.
    for (n = 0; n < symcount; n++) {
        int len = lens[n];
        if (len != 0) {
            int code = codes[len]++, slot = first[len]++;
            tree[slot] = (code << (32 - len)) | (n << 4) | len;
        }
    }

    return first[15];
}

static int decode(State* s, unsigned tree[], int max) {
    // Find the next prefix code.
    unsigned lo = 0, hi = max, key;
    unsigned search = (rev16(s->bits) << 16) | 0xffff;
    while (lo < hi) {
        unsigned guess = (lo + hi) / 2;
        if (search < tree[guess])
            hi = guess;
        else
            lo = guess + 1;
    }

    // Pull out the key and check it.
    key = tree[lo - 1];
    INFLATE_CHECK(((search ^ key) >> (32 - (key & 0xf))) == 0);

    bits(s, key & 0xf);
    return (key >> 4) & 0xfff;
}

static void run(State* s, int sym) {
    int length = bits(s, lenBits[sym]) + lenBase[sym];
    int dsym = decode(s, s->distcodes, s->tdist);
    int offs = bits(s, distBits[dsym]) + distBase[dsym];
    copy(s, s->out - offs, length);
}

static void block(State* s) {
    for (;;) {
        int sym = decode(s, s->litcodes, s->tlit);
        if (sym < 256)
            *emit(s, 1) = (unsigned char)sym;
        else if (sym > 256)
            run(s, sym - 257);
        else
            break;
    }
}

static void stored(State* s) {
    // Uncompressed data block.
    int len;
    bits(s, s->count & 7);
    len = bits(s, 16);
    INFLATE_CHECK(((len ^ s->bits) & 0xffff) == 0xffff);
    INFLATE_CHECK(s->in + len <= s->inend);

    copy(s, s->in, len);
    s->in += len;
    bits(s, 16);
}

static void fixed(State* s) {
    // Fixed set of Huffman codes.
    int n;
    unsigned char lens[288 + 32];
    for (n = 0; n <= 143; n++)
        lens[n] = 8;
    for (n = 144; n <= 255; n++)
        lens[n] = 9;
    for (n = 256; n <= 279; n++)
        lens[n] = 7;
    for (n = 280; n <= 287; n++)
        lens[n] = 8;
    for (n = 0; n < 32; n++)
        lens[288 + n] = 5;

    // Build lit/dist trees.
    s->tlit = build(s, s->litcodes, lens, 288);
    s->tdist = build(s, s->distcodes, lens + 288, 32);
}

static void dynamic(State* s) {
    int n, i, nlit, ndist, nlen;
    unsigned char lenlens[19] = { 0 }, lens[288 + 32];
    nlit = 257 + bits(s, 5);
    ndist = 1 + bits(s, 5);
    nlen = 4 + bits(s, 4);
    for (n = 0; n < nlen; n++)
        lenlens[(unsigned char)order[n]] = (unsigned char)bits(s, 3);

    // Build the tree for decoding code lengths.
    s->tlen = build(s, s->lencodes, lenlens, 19);

    // Decode code lengths.
    for (n = 0; n < nlit + ndist;) {
        int sym = decode(s, s->lencodes, s->tlen);
        switch (sym) {
            case 16:
                for (i = 3 + bits(s, 2); i; i--, n++)
                    lens[n] = lens[n - 1];
                break;
            case 17:
                for (i = 3 + bits(s, 3); i; i--, n++)
                    lens[n] = 0;
                break;
            case 18:
                for (i = 11 + bits(s, 7); i; i--, n++)
                    lens[n] = 0;
                break;
            default:
                lens[n++] = (unsigned char)sym;
                break;
        }
    }

    // Build lit/dist trees.
    s->tlit = build(s, s->litcodes, lens, nlit);
    s->tdist = build(s, s->distcodes, lens + nlit, ndist);
}

static int inflate(void* out, unsigned outlen, const void* in, unsigned inlen) {
    int last;
    State* s = calloc(1, sizeof(State));

    // We assume we can buffer 2 extra bytes from off the end of 'in'.
    s->in = (unsigned char*)in;
    s->inend = s->in + inlen + 2;
    s->out = (unsigned char*)out;
    s->outend = s->out + outlen;
    s->bits = 0;
    s->count = 0;
    bits(s, 0);

    if (setjmp(s->jmp) == 1) {
        free(s);
        return 0;
    }

    do {
        last = bits(s, 1);
        switch (bits(s, 2)) {
            case 0:
                stored(s);
                break;
            case 1:
                fixed(s);
                block(s);
                break;
            case 2:
                dynamic(s);
                block(s);
                break;
            case 3:
                INFLATE_FAIL();
        }
    } while (!last);

    free(s);
    return 1;
}

static Image* load_png(PNG *png) {
    const unsigned char *ihdr, *idat, *plte, *trns, *first;
    int trnsSize = 0;
    int depth, ctype, bipp;
    int datalen = 0;
    unsigned char *data = NULL, *out;
    Image *img = NULL;
    
    PNG_CHECK(memcmp(png->p, "\211PNG\r\n\032\n", 8) == 0);  // PNG signature
    png->p += 8;
    first = png->p;
    
    // Read IHDR
    ihdr = find(png, "IHDR", 13);
    PNG_CHECK(ihdr);
    depth = ihdr[8];
    ctype = ihdr[9];
    switch (ctype) {
        case 0:
            bipp = depth;
            break;  // greyscale
        case 2:
            bipp = 3 * depth;
            break;  // RGB
        case 3:
            bipp = depth;
            break;  // paletted
        case 4:
            bipp = 2 * depth;
            break;  // grey+alpha
        case 6:
            bipp = 4 * depth;
            break;  // RGBA
        default:
            PNG_FAIL();
    }
    
    // Allocate bitmap (+1 width to save room for stupid PNG filter bytes)
    img = CreateImage(get32(ihdr + 0) + 1, get32(ihdr + 4));
    PNG_CHECK(img->buf);
    img->w--;
    
    // We support 8-bit color components and 1, 2, 4 and 8 bit palette formats.
    // No interlacing, or wacky filter types.
    PNG_CHECK((depth != 16) && ihdr[10] == 0 && ihdr[11] == 0 && ihdr[12] == 0);
    
    // Join IDAT chunks.
    for (idat = find(png, "IDAT", 0); idat; idat = find(png, "IDAT", 0)) {
        unsigned len = get32(idat - 8);
        data = realloc(data, datalen + len);
        if (!data)
            break;
        
        memcpy(data + datalen, idat, len);
        datalen += len;
    }
    
    // Find palette.
    png->p = first;
    plte = find(png, "PLTE", 0);
    
    // Find transparency info.
    png->p = first;
    trns = find(png, "tRNS", 0);
    if (trns) {
        trnsSize = get32(trns - 8);
    }
    
    PNG_CHECK(data && datalen >= 6);
    PNG_CHECK((data[0] & 0x0f) == 0x08  // compression method (RFC 1950)
          && (data[0] & 0xf0) <= 0x70   // window size
          && (data[1] & 0x20) == 0);    // preset dictionary present
    
    out = (unsigned char*)img->buf + outsize(img, 32) - outsize(img, bipp);
    PNG_CHECK(inflate(out, outsize(img, bipp), data + 2, datalen - 6));
    PNG_CHECK(unfilter(img->w, img->h, bipp, out));
    
    if (ctype == 3) {
        PNG_CHECK(plte);
        depalette(img->w, img->h, out, img->buf, bipp, plte, trns, trnsSize);
    } else {
        PNG_CHECK(bipp % 8 == 0);
        convert(bipp / 8, img->w, img->h, out, img->buf, trns);
    }
    
    free(data);
    return img;
    
err:
    if (data)
        free(data);
    if (img && img->buf)
        DestroyImage(img);
    return false;
}


Image* LoadImageFromFile(const char *path) {
    char *data = NULL;
    size_t sizeOfData = 0;
    if (!(data = LoadFile(path, &sizeOfData)) && sizeOfData > 0)
        return false;
    Image *result = LoadImageFromMemory(data, sizeOfData);
    free(data);
    return result;
}

Image* LoadImageFromMemory(const void *data, size_t length) {
    PNG png = {
        .p = (unsigned char*)data,
        .end = (unsigned char*)data + length
    };
    return load_png(&png);
}

typedef struct {
    unsigned crc, adler, bits, prev, runlen;
    FILE* out;
    unsigned crcTable[256];
} Save;

static const unsigned crctable[16] = { 0,          0x1db71064, 0x3b6e20c8, 0x26d930ac, 0x76dc4190, 0x6b6b51f4,
                                       0x4db26158, 0x5005713c, 0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
                                       0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c };

static void put(Save* s, unsigned v) {
    fputc(v, s->out);
    s->crc = (s->crc >> 4) ^ crctable[(s->crc & 15) ^ (v & 15)];
    s->crc = (s->crc >> 4) ^ crctable[(s->crc & 15) ^ (v >> 4)];
}

static void updateAdler(Save* s, unsigned v) {
    unsigned s1 = s->adler & 0xffff, s2 = (s->adler >> 16) & 0xffff;
    s1 = (s1 + v) % 65521;
    s2 = (s2 + s1) % 65521;
    s->adler = (s2 << 16) + s1;
}

static void put32(Save* s, unsigned v) {
    put(s, (v >> 24) & 0xff);
    put(s, (v >> 16) & 0xff);
    put(s, (v >> 8) & 0xff);
    put(s, v & 0xff);
}

void putbits(Save* s, unsigned data, unsigned bitcount) {
    while (bitcount--) {
        unsigned prev = s->bits;
        s->bits = (s->bits >> 1) | ((data & 1) << 7);
        data >>= 1;
        if (prev & 1) {
            put(s, s->bits);
            s->bits = 0x80;
        }
    }
}

void putbitsr(Save* s, unsigned data, unsigned bitcount) {
    while (bitcount--)
        putbits(s, data >> bitcount, 1);
}

static void begin(Save* s, const char* id, unsigned len) {
    put32(s, len);
    s->crc = 0xffffffff;
    put(s, id[0]);
    put(s, id[1]);
    put(s, id[2]);
    put(s, id[3]);
}

static void literal(Save* s, unsigned v) {
    // Encode a literal/length using the built-in tables.
    // Could do better with a custom table but whatever.
    if (v < 144)
        putbitsr(s, 0x030 + v - 0, 8);
    else if (v < 256)
        putbitsr(s, 0x190 + v - 144, 9);
    else if (v < 280)
        putbitsr(s, 0x000 + v - 256, 7);
    else
        putbitsr(s, 0x0c0 + v - 280, 8);
}

static void encodelen(Save* s, unsigned code, unsigned bits, unsigned len) {
    literal(s, code + (len >> bits));
    putbits(s, len, bits);
    putbits(s, 0, 5);
}

static void endrun(Save* s) {
    s->runlen--;
    literal(s, s->prev);

    if (s->runlen >= 67)
        encodelen(s, 277, 4, s->runlen - 67);
    else if (s->runlen >= 35)
        encodelen(s, 273, 3, s->runlen - 35);
    else if (s->runlen >= 19)
        encodelen(s, 269, 2, s->runlen - 19);
    else if (s->runlen >= 11)
        encodelen(s, 265, 1, s->runlen - 11);
    else if (s->runlen >= 3)
        encodelen(s, 257, 0, s->runlen - 3);
    else
        while (s->runlen--)
            literal(s, s->prev);
}

static void encodeByte(Save* s, unsigned char v) {
    updateAdler(s, v);

    // Simple RLE compression. We could do better by doing a search
    // to find matches, but this works pretty well TBH.
    if (s->prev == v && s->runlen < 115) {
        s->runlen++;
    } else {
        if (s->runlen)
            endrun(s);

        s->prev = v;
        s->runlen = 1;
    }
}

static void savePngHeader(Save* s, Image* bmp) {
    fwrite("\211PNG\r\n\032\n", 8, 1, s->out);
    begin(s, "IHDR", 13);
    put32(s, bmp->w);
    put32(s, bmp->h);
    put(s, 8);  // bit depth
    put(s, 6);  // RGBA
    put(s, 0);  // compression (deflate)
    put(s, 0);  // filter (standard)
    put(s, 0);  // interlace off
    put32(s, ~s->crc);
}

static long savePngData(Save* s, Image* bmp, long dataPos) {
    int x, y;
    long dataSize;
    begin(s, "IDAT", 0);
    put(s, 0x08);      // zlib compression method
    put(s, 0x1d);      // zlib compression flags
    putbits(s, 3, 3);  // zlib last block + fixed dictionary
    for (y = 0; y < bmp->h; y++) {
        int *row = &bmp->buf[y * bmp->w];
        Color prev = {0};

        encodeByte(s, 1);  // sub filter
        for (x = 0; x < bmp->w; x++) {
            Color col = {.rgba = row[x]};
            encodeByte(s, col.r - prev.r);
            encodeByte(s, col.g - prev.g);
            encodeByte(s, col.b - prev.b);
            encodeByte(s, col.a - prev.a);
            prev = col;
        }
    }
    endrun(s);
    literal(s, 256);  // terminator
    while (s->bits != 0x80)
        putbits(s, 0, 1);
    put32(s, s->adler);
    dataSize = (ftell(s->out) - dataPos) - 8;
    put32(s, ~s->crc);
    return dataSize;
}

bool SaveImage(Image *img, const char *path) {
    FILE* out = fopen(path, "wb");
    if (!out)
        return false;
    
    Save s;
    s.out = out;
    s.adler = 1;
    s.bits = 0x80;
    s.prev = 0xffff;
    s.runlen = 0;
    
    savePngHeader(&s, img);
    long dataPos = ftell(s.out);
    long dataSize = savePngData(&s, img, dataPos);
    
    // End chunk.
    begin(&s, "IEND", 0);
    put32(&s, ~s.crc);
    
    // Write back payload size.
    fseek(out, dataPos, SEEK_SET);
    put32(&s, dataSize);
    
    long err = ferror(out);
    fclose(out);
    return !err;
}

static Texture* NewTexture(sg_image_desc *desc) {
    Texture *result = malloc(sizeof(Texture));
    result->sg = sg_make_image(desc);
    result->w = desc->width;
    result->h = desc->height;
    return result;
}

Texture* LoadTextureFromImage(Image *img) {
    sg_image_desc desc = {
        .width = img->w,
        .height = img->h,
        .data.subimage[0][0] = {
            .ptr= img->buf,
            .size= img->w * img->h * sizeof(int)
        }
    };
    return NewTexture(&desc);
}

Texture* LoadTextureFromFile(const char *path) {
    Image *img = LoadImageFromFile(path);
    Texture *result = LoadTextureFromImage(img);
    DestroyImage(img);
    return result;
}

Texture* CreateMutableTexture(int w, int h) {
    sg_image_desc desc = {
        .width = w,
        .height = h,
        .usage = SG_USAGE_STREAM
    };
    return NewTexture(&desc);
}

void UpdateMutableTexture(Texture *texture, Image *img) {
    if (texture->w != img->w || texture->h != img->h) {
        DestroyTexture(texture);
        texture = CreateMutableTexture(img->w, img->h);
    }
    sg_image_data data = {
        .subimage[0][0] = (sg_range) {
            .ptr = img->buf,
            .size = img->w * img->h * sizeof(int)
        }
    };
    sg_update_image(texture->sg, &data);
}

typedef Vertex Quad[6];

static void GenerateQuad(Vector2 position, Vector2 textureSize, Vector2 size, Vector2 scale, Vector2 viewportSize, float rotation, Rectangle clip, Quad *out) {
    Vector2 scaledSize = {size.x * scale.x, size.y * scale.y};
    Vector2 quad[4] = {
        {position.x, position.y + scaledSize.y}, // bottom left
        {position.x + scaledSize.x, position.y + scaledSize.y}, // bottom right
        {position.x + scaledSize.x, position.y }, // top right
        {position.x, position.y }, // top left
    };
    float vw =  2.f / (float)viewportSize.x;
    float vh = -2.f / (float)viewportSize.y;
    for (int j = 0; j < 4; j++)
        quad[j] = (Vector2) {
            vw * quad[j].x + -1.f,
            vh * quad[j].y +  1.f
        };
    
    float iw = 1.f/textureSize.x, ih = 1.f/(float)textureSize.y;
    float tl = clip.x*iw;
    float tt = clip.y*ih;
    float tr = (clip.x + clip.w)*iw;
    float tb = (clip.y + clip.h)*ih;
    Vector2 vtexquad[4] = {
        {tl, tb}, // bottom left
        {tr, tb}, // bottom right
        {tr, tt}, // top right
        {tl, tt}, // top left
    };
    
    static int indices[6] = {
        0, 1, 2,
        3, 0, 2
    };
    
    for (int i = 0; i < 6; i++)
        (*out)[i] = (Vertex) {
            .position = quad[indices[i]],
            .texcoord = vtexquad[indices[i]],
            .color = {0.f, 0.f, 0.f, 1.f}
        };
}

void DrawTexture(Texture *texture, Vector2 position, Vector2 size, Vector2 scale, Vector2 viewportSize, float rotation, Rectangle clip) {
    Quad quad;
    GenerateQuad(position, (Vector2){texture->w, texture->h}, size, scale, viewportSize, rotation, clip, &quad);
    sg_buffer_desc desc = {
        .data = SG_RANGE(quad)
    };
    sg_bindings bind = {
        .vertex_buffers[0] = sg_make_buffer(&desc),
        .fs_images[SLOT_sprite] = texture->sg
    };
    sg_apply_bindings(&bind);
    sg_draw(0, 6, 1);
    sg_destroy_buffer(bind.vertex_buffers[0]);
}

void DestroyTexture(Texture *texture) {
    if (texture) {
        if (sg_query_image_state(texture->sg) == SG_RESOURCESTATE_VALID)
            sg_destroy_image(texture->sg);
        free(texture);
    }
}

TextureBatch* CreateTextureBatch(Texture *texture, int maxVertices) {
    TextureBatch *result = malloc(sizeof(TextureBatch));
    result->maxVertices = maxVertices;
    result->vertexCount = 0;
    result->size = (Vector2){texture->w, texture->h};
    size_t sz = 6 * maxVertices * sizeof(Vertex);
    result->vertices = malloc(sz);
    sg_buffer_desc desc = {
        .usage = SG_USAGE_STREAM,
        .size = sz
    };
    result->bind = (sg_bindings) {
        .vertex_buffers[0] = sg_make_buffer(&desc),
        .fs_images[SLOT_sprite] = texture->sg
    };
    return result;
}

void TextureBatchDraw(TextureBatch *batch, Vector2 position, Vector2 size, Vector2 scale, Vector2 viewportSize, float rotation, Rectangle clip) {
    Quad quad;
    GenerateQuad(position, batch->size, size, scale, viewportSize, rotation, clip, &quad);
    memcpy(&batch->vertices[batch->vertexCount++ * 6], &quad, 6 * sizeof(Vertex));
}

void FlushTextureBatch(TextureBatch *batch) {
    sg_update_buffer(batch->bind.vertex_buffers[0], &(sg_range) {
        .ptr = batch->vertices,
        .size = 6 * batch->vertexCount * sizeof(Vertex)
    });
    sg_apply_bindings(&batch->bind);
    sg_draw(0, 6 * batch->vertexCount, 1);
    memset(batch->vertices, 0, 6 * batch->maxVertices * sizeof(Vertex));
    batch->vertexCount = 0;
}

void DestroyTextureBatch(TextureBatch *batch) {
    if (batch) {
        if (batch->vertices)
            free(batch->vertices);
        if (sg_query_buffer_state(batch->bind.vertex_buffers[0]) == SG_RESOURCESTATE_VALID)
            sg_destroy_buffer(batch->bind.vertex_buffers[0]);
        free(batch);
    }
}

static void InitCallback(void) {
    if (state.mouseHidden)
        sapp_show_mouse(false);
    if (state.mouseLocked)
        sapp_lock_mouse(true);
    
    sg_desc desc = (sg_desc) {
        .context = sapp_sgcontext()
    };
    sg_setup(&desc);
    
    sg_image_desc img_desc = {
        .render_target = true,
        .width = 640,
        .height = 480,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .min_filter = SG_FILTER_NEAREST,
        .mag_filter = SG_FILTER_NEAREST,
        .wrap_u = SG_WRAP_REPEAT,
        .wrap_v = SG_WRAP_REPEAT
    };
    state.color = sg_make_image(&img_desc);
    img_desc.pixel_format = SG_PIXELFORMAT_DEPTH;
    state.depth = sg_make_image(&img_desc);
    state.pass = sg_make_pass(&(sg_pass_desc){
        .color_attachments[0].image = state.color,
        .depth_stencil_attachment.image = state.depth
    });
    
    const float vertices[] = {
        // pos      // uv
        -1.f,  1.f, 0.f, 1.f,
         1.f,  1.f, 1.f, 1.f,
         1.f, -1.f, 1.f, 0.f,
        -1.f, -1.f, 0.f, 0.f,
    };
    const uint16_t indices[] = {
        0, 1, 2,
        0, 2, 3
    };
    state.bind = (sg_bindings) {
        .vertex_buffers[0] =  sg_make_buffer(&(sg_buffer_desc){
            .data = SG_RANGE(vertices)
        }),
        .index_buffer = sg_make_buffer(&(sg_buffer_desc){
            .type = SG_BUFFERTYPE_INDEXBUFFER,
            .data = SG_RANGE(indices),
        }),
        .fs_images = state.color
    };
    
    sg_pipeline_desc framebuffer_desc = {
        .shader = sg_make_shader(framebuffer_program_shader_desc(sg_query_backend())),
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
        .index_type = SG_INDEXTYPE_UINT16,
        .layout = {
            .attrs = {
                [0].format = SG_VERTEXFORMAT_FLOAT2,
                [1].format = SG_VERTEXFORMAT_FLOAT2,
            }
        },
        .depth = {
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled = true
        },
        .cull_mode = SG_CULLMODE_BACK,
    };
    state.framebuffer_pip = sg_make_pipeline(&framebuffer_desc);
    
    sg_pipeline_desc offscreen_desc = {
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
        .depth = {
            .pixel_format = SG_PIXELFORMAT_DEPTH,
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled = true,
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
            },
            .pixel_format = SG_PIXELFORMAT_RGBA8
        }
    };
    state.offscreen_pip = sg_make_pipeline(&offscreen_desc);
    
    if (state.init_cb)
        state.init_cb();
}

static void FrameCallback(void) {
    sg_begin_pass(state.pass, &state.pass_action);
    sg_apply_pipeline(state.offscreen_pip);
    if (state.frame_cb)
        state.frame_cb();
    sg_end_pass();
    
    sg_begin_default_pass(&state.pass_action, 640, 480);
    sg_apply_pipeline(state.framebuffer_pip);
    sg_apply_bindings(&state.bind);
    sg_draw(0, 6, 1);
    sg_end_pass();
    
    sg_commit();
    
    state.input.mouse_delta = state.input.mouse_scroll_delta = (Vector2){0};
    for (int i = 0; i < SAPP_MAX_KEYCODES; i++)
        if (state.input.button_clicked[i])
            state.input.button_clicked[i] = false;
    for (int i = 0; i < SAPP_MAX_MOUSEBUTTONS; i++)
        if (state.input.mouse_clicked[i])
            state.input.mouse_clicked[i] = false;
}

static void EventCallback(const sapp_event *e) {
    if (state.event_cb)
        state.event_cb(e);
    
    switch (e->type) {
        case SAPP_EVENTTYPE_KEY_DOWN:
            state.input.button_down[e->key_code] = true;
            break;
        case SAPP_EVENTTYPE_KEY_UP:
            state.input.button_down[e->key_code] = false;
            state.input.button_clicked[e->key_code] = true;
            break;
        case SAPP_EVENTTYPE_MOUSE_DOWN:
            state.input.mouse_down[e->mouse_button] = true;
            break;
        case SAPP_EVENTTYPE_MOUSE_UP:
            state.input.mouse_down[e->mouse_button] = false;
            state.input.mouse_clicked[e->mouse_button] = true;
            break;
        case SAPP_EVENTTYPE_MOUSE_MOVE:
            state.input.last_mouse_pos = state.input.mouse_pos;
            state.input.mouse_pos = (Vector2){e->mouse_x, e->mouse_y};
            state.input.mouse_delta = (Vector2){e->mouse_dx, e->mouse_dy};
            break;
        case SAPP_EVENTTYPE_MOUSE_SCROLL:
            state.input.mouse_scroll_delta = (Vector2){e->scroll_x, e->scroll_y};
            break;
        default:
            break;
    }
}

static void CleanupCallback(void) {
    state.running = false;
    if (state.cleanup_cb)
        state.cleanup_cb();
    sg_shutdown();
}

int OrderUp(const sapp_desc *desc) {
    memcpy(&state.desc, desc, sizeof(sapp_desc));
    state.init_cb = desc->init_cb;
    state.frame_cb = desc->frame_cb;
    state.event_cb = desc->event_cb;
    state.cleanup_cb = desc->cleanup_cb;
    state.desc.init_cb = InitCallback;
    state.desc.frame_cb = FrameCallback;
    state.desc.event_cb = EventCallback;
    state.desc.cleanup_cb = CleanupCallback;
    sapp_run(&state.desc);
    return EXIT_SUCCESS;
}
