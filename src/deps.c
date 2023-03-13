//
//  deps.c
//  colony
//
//  Created by George Watson on 08/02/2023.
//

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_IMPLEMENTATION
#include "nuklear.h"
#define SOKOL_IMPL
#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sokol_glue.h"
#include "sokol_args.h"
#include "sokol_time.h"
#include "sokol_nuklear.h"
#define HASHMAP_IMPL
#include "hashmap.h"
#define MJSON_IMPLEMENTATION
#include "mjson.h"
