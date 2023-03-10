//
//  gui.c
//  colony
//
//  Created by George Watson on 10/03/2023.
//

#include "gui.h"

typedef struct {
    const char *name;
    struct nk_vec2 position, size;
    bool open;
    nk_flags flags;
    WindowCb update;
    sapp_keycode key;
    uint32_t modifier;
} WindowBucket;

static struct {
    struct hashmap *map;
} WindowManager;

static uint64_t HashWindow(const void *item, uint64_t seed0, uint64_t seed1) {
    WindowBucket *bucket = (WindowBucket*)item;
    return hashmap_sip(bucket->name, strlen(bucket->name), seed0, seed1);
}

static int CompareWindow(const void *a, const void *b, void*_) {
    WindowBucket *bucketA = (WindowBucket*)a;
    WindowBucket *bucketB = (WindowBucket*)b;
    return strcmp(bucketA->name, bucketB->name);
}

void InitWindowManager(void) {
    WindowManager.map = hashmap_new(sizeof(WindowBucket), 0, 0, 0, HashWindow, CompareWindow, NULL, NULL);
}

void WindowManagerAdd(const char *name, struct nk_vec2 position, struct nk_vec2 size, bool open, nk_flags flags, WindowCb cb, sapp_keycode key) {
    WindowBucket search = {.name=name};
    WindowBucket *found = NULL;
    if ((found = hashmap_get(WindowManager.map, (void*)&search)))
        return;
    search.position = position;
    search.size = size;
    search.open = open;
    search.flags = flags;
    search.update = cb;
    search.key = key;
    search.name = name;
    hashmap_set(WindowManager.map, (void*)&search);
}

void WindowManagerUpdate(struct nk_context *ctx) {
    size_t iter = 0;
    void *item;
    while (hashmap_iter(WindowManager.map, &iter, &item)) {
        WindowBucket *window = (WindowBucket*)item;
        bool closed = nk_window_is_closed(ctx, window->name);
        if (window->open && closed)
            window->open = false;
        if (WasKeyPressed(window->key))
            window->open = !window->open;
        
        if (window->open && nk_begin(ctx, window->name, nk_rect(window->position.x, window->position.y, window->size.x, window->size.y), window->flags)) {
            window->update(ctx);
            window->position = nk_window_get_position(ctx);
            window->size = nk_window_get_size(ctx);
        }
        if (ctx->current)
            nk_end(ctx);
    }
}

void DestroyWindowManager(void) {
    if (WindowManager.map)
        hashmap_free(WindowManager.map);
}
