#include "miso.h"
#include "assets/sprite.glsl.h"
#include "assets/framebuffer.glsl.h"

#define MAP_WIDTH 256
#define MAP_HEIGHT 256

static struct {
    sg_pass_action pass_action;
    sg_pass pass;
    sg_pipeline pip;
    sg_bindings bind;
    sg_image color, depth;
    
    Texture *texture;
    TextureBatch *tbatch;
    sg_pass_action pass_action2;
    sg_pipeline pip2;
} state;

static void init(void) {
    sg_setup(&(sg_desc){
        .context = sapp_sgcontext(),
        .buffer_pool_size = 256
    });
    
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
    
    state.pip = sg_make_pipeline(&(sg_pipeline_desc){
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
    });
    
    state.pass_action.colors[0] = (sg_color_attachment_action) {
        .action = SG_ACTION_CLEAR,
        .value = {0.f, 0.f, 0.f, 1.f}
    };
    
    state.pip2 = sg_make_pipeline(&(sg_pipeline_desc) {
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
    });
    
    state.pass_action2.colors[0] = (sg_color_attachment_action) {
        .action = SG_ACTION_CLEAR,
        .value = {0.f, 0.f, 0.f, 1.f}
    };
    
    Image *test = LoadImageFromFile("assets/tiles.png");
    state.texture = LoadTextureFromImage(test);
    state.tbatch = CreateTextureBatch(state.texture, 1);
    
    DestroyImage(test);
}

static void frame(void) {
    sg_begin_pass(state.pass, &state.pass_action2);
    sg_apply_pipeline(state.pip2);
    TextureBatchDraw(state.tbatch, (Vector2){50, 50}, (Vector2){state.texture->w, state.texture->h}, (Vector2){1.f, 1.f}, (Vector2){640, 480}, 1.f, (Rectangle){0, 0, state.texture->w, state.texture->h});
    FlushTextureBatch(state.tbatch);
    sg_end_pass();
    
    sg_begin_default_pass(&state.pass_action, 640, 480);
    sg_apply_pipeline(state.pip);
    sg_apply_bindings(&state.bind);
    sg_draw(0, 6, 1);
    sg_end_pass();
    
    sg_commit();
}

static void event(const sapp_event *event) {
    
}

static void cleanup(void) {
}

int main(int argc, const char *argv[]) {
    sapp_desc desc = {
        .width = 640,
        .height = 480,
        .window_title = "miso!",
        .init_cb = init,
        .frame_cb = frame,
        .event_cb = event,
        .cleanup_cb = cleanup
    };
    return OrderUp(&desc);
}
