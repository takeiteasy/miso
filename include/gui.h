//
//  gui.h
//  colony
//
//  Created by George Watson on 10/03/2023.
//

#ifndef gui_h
#define gui_h
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_STANDARD_VARARGS
#include "nuklear.h"
#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sokol_nuklear.h"
#include "hashmap.h"
#include "input.h"
#include <string.h>

typedef void(*WindowCb)(struct nk_context*);

void InitWindowManager(void);
void WindowManagerAdd(const char *name, struct nk_vec2 position, struct nk_vec2 size, bool open, nk_flags flags, WindowCb cb, sapp_keycode key);
void WindowManagerUpdate(struct nk_context *ctx);
void DestroyWindowManager(void);

#endif /* gui_h */
