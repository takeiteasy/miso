#pragma once
/*
    #version:1# (machine generated, don't edit!)

    Generated by sokol-shdc (https://github.com/floooh/sokol-tools)

    Cmdline: sokol-shdc -i assets/sprite.glsl -o assets/sprite.glsl.h -l metal_macos

    Overview:

        Shader program 'sprite_program':
            Get shader desc: sprite_program_shader_desc(sg_query_backend());
            Vertex shader: sprite_vs
                Attribute slots:
                    ATTR_sprite_vs_position = 0
                    ATTR_sprite_vs_texcoord = 1
                    ATTR_sprite_vs_color = 2
            Fragment shader: sprite_fs
                Image 'sprite':
                    Type: SG_IMAGETYPE_2D
                    Component Type: SG_SAMPLERTYPE_FLOAT
                    Bind slot: SLOT_sprite = 0


    Shader descriptor structs:

        sg_shader sprite_program = sg_make_shader(sprite_program_shader_desc(sg_query_backend()));

    Vertex attribute locations for vertex shader 'sprite_vs':

        sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
            .layout = {
                .attrs = {
                    [ATTR_sprite_vs_position] = { ... },
                    [ATTR_sprite_vs_texcoord] = { ... },
                    [ATTR_sprite_vs_color] = { ... },
                },
            },
            ...});

    Image bind slots, use as index in sg_bindings.vs_images[] or .fs_images[]

        SLOT_sprite = 0;

*/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>
#if !defined(SOKOL_SHDC_ALIGN)
  #if defined(_MSC_VER)
    #define SOKOL_SHDC_ALIGN(a) __declspec(align(a))
  #else
    #define SOKOL_SHDC_ALIGN(a) __attribute__((aligned(a)))
  #endif
#endif
#define ATTR_sprite_vs_position (0)
#define ATTR_sprite_vs_texcoord (1)
#define ATTR_sprite_vs_color (2)
#define SLOT_sprite (0)
/*
    #include <metal_stdlib>
    #include <simd/simd.h>
    
    using namespace metal;
    
    struct main0_out
    {
        float2 uv [[user(locn0)]];
        float4 col [[user(locn1)]];
        float4 gl_Position [[position]];
    };
    
    struct main0_in
    {
        float2 position [[attribute(0)]];
        float2 texcoord [[attribute(1)]];
        float4 color [[attribute(2)]];
    };
    
    #line 14 "assets/sprite.glsl"
    vertex main0_out main0(main0_in in [[stage_in]])
    {
        main0_out out = {};
    #line 14 "assets/sprite.glsl"
        out.gl_Position = float4(in.position, 0.0, 1.0);
    #line 15 "assets/sprite.glsl"
        out.uv = in.texcoord;
    #line 16 "assets/sprite.glsl"
        out.col = in.color;
        return out;
    }
    
*/
static const char sprite_vs_source_metal_macos[646] = {
    0x23,0x69,0x6e,0x63,0x6c,0x75,0x64,0x65,0x20,0x3c,0x6d,0x65,0x74,0x61,0x6c,0x5f,
    0x73,0x74,0x64,0x6c,0x69,0x62,0x3e,0x0a,0x23,0x69,0x6e,0x63,0x6c,0x75,0x64,0x65,
    0x20,0x3c,0x73,0x69,0x6d,0x64,0x2f,0x73,0x69,0x6d,0x64,0x2e,0x68,0x3e,0x0a,0x0a,
    0x75,0x73,0x69,0x6e,0x67,0x20,0x6e,0x61,0x6d,0x65,0x73,0x70,0x61,0x63,0x65,0x20,
    0x6d,0x65,0x74,0x61,0x6c,0x3b,0x0a,0x0a,0x73,0x74,0x72,0x75,0x63,0x74,0x20,0x6d,
    0x61,0x69,0x6e,0x30,0x5f,0x6f,0x75,0x74,0x0a,0x7b,0x0a,0x20,0x20,0x20,0x20,0x66,
    0x6c,0x6f,0x61,0x74,0x32,0x20,0x75,0x76,0x20,0x5b,0x5b,0x75,0x73,0x65,0x72,0x28,
    0x6c,0x6f,0x63,0x6e,0x30,0x29,0x5d,0x5d,0x3b,0x0a,0x20,0x20,0x20,0x20,0x66,0x6c,
    0x6f,0x61,0x74,0x34,0x20,0x63,0x6f,0x6c,0x20,0x5b,0x5b,0x75,0x73,0x65,0x72,0x28,
    0x6c,0x6f,0x63,0x6e,0x31,0x29,0x5d,0x5d,0x3b,0x0a,0x20,0x20,0x20,0x20,0x66,0x6c,
    0x6f,0x61,0x74,0x34,0x20,0x67,0x6c,0x5f,0x50,0x6f,0x73,0x69,0x74,0x69,0x6f,0x6e,
    0x20,0x5b,0x5b,0x70,0x6f,0x73,0x69,0x74,0x69,0x6f,0x6e,0x5d,0x5d,0x3b,0x0a,0x7d,
    0x3b,0x0a,0x0a,0x73,0x74,0x72,0x75,0x63,0x74,0x20,0x6d,0x61,0x69,0x6e,0x30,0x5f,
    0x69,0x6e,0x0a,0x7b,0x0a,0x20,0x20,0x20,0x20,0x66,0x6c,0x6f,0x61,0x74,0x32,0x20,
    0x70,0x6f,0x73,0x69,0x74,0x69,0x6f,0x6e,0x20,0x5b,0x5b,0x61,0x74,0x74,0x72,0x69,
    0x62,0x75,0x74,0x65,0x28,0x30,0x29,0x5d,0x5d,0x3b,0x0a,0x20,0x20,0x20,0x20,0x66,
    0x6c,0x6f,0x61,0x74,0x32,0x20,0x74,0x65,0x78,0x63,0x6f,0x6f,0x72,0x64,0x20,0x5b,
    0x5b,0x61,0x74,0x74,0x72,0x69,0x62,0x75,0x74,0x65,0x28,0x31,0x29,0x5d,0x5d,0x3b,
    0x0a,0x20,0x20,0x20,0x20,0x66,0x6c,0x6f,0x61,0x74,0x34,0x20,0x63,0x6f,0x6c,0x6f,
    0x72,0x20,0x5b,0x5b,0x61,0x74,0x74,0x72,0x69,0x62,0x75,0x74,0x65,0x28,0x32,0x29,
    0x5d,0x5d,0x3b,0x0a,0x7d,0x3b,0x0a,0x0a,0x23,0x6c,0x69,0x6e,0x65,0x20,0x31,0x34,
    0x20,0x22,0x61,0x73,0x73,0x65,0x74,0x73,0x2f,0x73,0x70,0x72,0x69,0x74,0x65,0x2e,
    0x67,0x6c,0x73,0x6c,0x22,0x0a,0x76,0x65,0x72,0x74,0x65,0x78,0x20,0x6d,0x61,0x69,
    0x6e,0x30,0x5f,0x6f,0x75,0x74,0x20,0x6d,0x61,0x69,0x6e,0x30,0x28,0x6d,0x61,0x69,
    0x6e,0x30,0x5f,0x69,0x6e,0x20,0x69,0x6e,0x20,0x5b,0x5b,0x73,0x74,0x61,0x67,0x65,
    0x5f,0x69,0x6e,0x5d,0x5d,0x29,0x0a,0x7b,0x0a,0x20,0x20,0x20,0x20,0x6d,0x61,0x69,
    0x6e,0x30,0x5f,0x6f,0x75,0x74,0x20,0x6f,0x75,0x74,0x20,0x3d,0x20,0x7b,0x7d,0x3b,
    0x0a,0x23,0x6c,0x69,0x6e,0x65,0x20,0x31,0x34,0x20,0x22,0x61,0x73,0x73,0x65,0x74,
    0x73,0x2f,0x73,0x70,0x72,0x69,0x74,0x65,0x2e,0x67,0x6c,0x73,0x6c,0x22,0x0a,0x20,
    0x20,0x20,0x20,0x6f,0x75,0x74,0x2e,0x67,0x6c,0x5f,0x50,0x6f,0x73,0x69,0x74,0x69,
    0x6f,0x6e,0x20,0x3d,0x20,0x66,0x6c,0x6f,0x61,0x74,0x34,0x28,0x69,0x6e,0x2e,0x70,
    0x6f,0x73,0x69,0x74,0x69,0x6f,0x6e,0x2c,0x20,0x30,0x2e,0x30,0x2c,0x20,0x31,0x2e,
    0x30,0x29,0x3b,0x0a,0x23,0x6c,0x69,0x6e,0x65,0x20,0x31,0x35,0x20,0x22,0x61,0x73,
    0x73,0x65,0x74,0x73,0x2f,0x73,0x70,0x72,0x69,0x74,0x65,0x2e,0x67,0x6c,0x73,0x6c,
    0x22,0x0a,0x20,0x20,0x20,0x20,0x6f,0x75,0x74,0x2e,0x75,0x76,0x20,0x3d,0x20,0x69,
    0x6e,0x2e,0x74,0x65,0x78,0x63,0x6f,0x6f,0x72,0x64,0x3b,0x0a,0x23,0x6c,0x69,0x6e,
    0x65,0x20,0x31,0x36,0x20,0x22,0x61,0x73,0x73,0x65,0x74,0x73,0x2f,0x73,0x70,0x72,
    0x69,0x74,0x65,0x2e,0x67,0x6c,0x73,0x6c,0x22,0x0a,0x20,0x20,0x20,0x20,0x6f,0x75,
    0x74,0x2e,0x63,0x6f,0x6c,0x20,0x3d,0x20,0x69,0x6e,0x2e,0x63,0x6f,0x6c,0x6f,0x72,
    0x3b,0x0a,0x20,0x20,0x20,0x20,0x72,0x65,0x74,0x75,0x72,0x6e,0x20,0x6f,0x75,0x74,
    0x3b,0x0a,0x7d,0x0a,0x0a,0x00,
};
/*
    #include <metal_stdlib>
    #include <simd/simd.h>
    
    using namespace metal;
    
    struct main0_out
    {
        float4 fragColor [[color(0)]];
    };
    
    struct main0_in
    {
        float2 uv [[user(locn0)]];
        float4 col [[user(locn1)]];
    };
    
    #line 13 "assets/sprite.glsl"
    fragment main0_out main0(main0_in in [[stage_in]], texture2d<float> sprite [[texture(0)]], sampler spriteSmplr [[sampler(0)]])
    {
        main0_out out = {};
    #line 13 "assets/sprite.glsl"
        float4 _21 = sprite.sample(spriteSmplr, in.uv);
    #line 14 "assets/sprite.glsl"
        float4 _32;
        if (_21.w > 0.0)
        {
            _32 = select(in.col, _21, bool4(all(in.col == float4(0.0, 0.0, 0.0, 1.0))));
        }
        else
        {
            _32 = float4(0.0);
        }
        out.fragColor = _32;
        return out;
    }
    
*/
static const char sprite_fs_source_metal_macos[737] = {
    0x23,0x69,0x6e,0x63,0x6c,0x75,0x64,0x65,0x20,0x3c,0x6d,0x65,0x74,0x61,0x6c,0x5f,
    0x73,0x74,0x64,0x6c,0x69,0x62,0x3e,0x0a,0x23,0x69,0x6e,0x63,0x6c,0x75,0x64,0x65,
    0x20,0x3c,0x73,0x69,0x6d,0x64,0x2f,0x73,0x69,0x6d,0x64,0x2e,0x68,0x3e,0x0a,0x0a,
    0x75,0x73,0x69,0x6e,0x67,0x20,0x6e,0x61,0x6d,0x65,0x73,0x70,0x61,0x63,0x65,0x20,
    0x6d,0x65,0x74,0x61,0x6c,0x3b,0x0a,0x0a,0x73,0x74,0x72,0x75,0x63,0x74,0x20,0x6d,
    0x61,0x69,0x6e,0x30,0x5f,0x6f,0x75,0x74,0x0a,0x7b,0x0a,0x20,0x20,0x20,0x20,0x66,
    0x6c,0x6f,0x61,0x74,0x34,0x20,0x66,0x72,0x61,0x67,0x43,0x6f,0x6c,0x6f,0x72,0x20,
    0x5b,0x5b,0x63,0x6f,0x6c,0x6f,0x72,0x28,0x30,0x29,0x5d,0x5d,0x3b,0x0a,0x7d,0x3b,
    0x0a,0x0a,0x73,0x74,0x72,0x75,0x63,0x74,0x20,0x6d,0x61,0x69,0x6e,0x30,0x5f,0x69,
    0x6e,0x0a,0x7b,0x0a,0x20,0x20,0x20,0x20,0x66,0x6c,0x6f,0x61,0x74,0x32,0x20,0x75,
    0x76,0x20,0x5b,0x5b,0x75,0x73,0x65,0x72,0x28,0x6c,0x6f,0x63,0x6e,0x30,0x29,0x5d,
    0x5d,0x3b,0x0a,0x20,0x20,0x20,0x20,0x66,0x6c,0x6f,0x61,0x74,0x34,0x20,0x63,0x6f,
    0x6c,0x20,0x5b,0x5b,0x75,0x73,0x65,0x72,0x28,0x6c,0x6f,0x63,0x6e,0x31,0x29,0x5d,
    0x5d,0x3b,0x0a,0x7d,0x3b,0x0a,0x0a,0x23,0x6c,0x69,0x6e,0x65,0x20,0x31,0x33,0x20,
    0x22,0x61,0x73,0x73,0x65,0x74,0x73,0x2f,0x73,0x70,0x72,0x69,0x74,0x65,0x2e,0x67,
    0x6c,0x73,0x6c,0x22,0x0a,0x66,0x72,0x61,0x67,0x6d,0x65,0x6e,0x74,0x20,0x6d,0x61,
    0x69,0x6e,0x30,0x5f,0x6f,0x75,0x74,0x20,0x6d,0x61,0x69,0x6e,0x30,0x28,0x6d,0x61,
    0x69,0x6e,0x30,0x5f,0x69,0x6e,0x20,0x69,0x6e,0x20,0x5b,0x5b,0x73,0x74,0x61,0x67,
    0x65,0x5f,0x69,0x6e,0x5d,0x5d,0x2c,0x20,0x74,0x65,0x78,0x74,0x75,0x72,0x65,0x32,
    0x64,0x3c,0x66,0x6c,0x6f,0x61,0x74,0x3e,0x20,0x73,0x70,0x72,0x69,0x74,0x65,0x20,
    0x5b,0x5b,0x74,0x65,0x78,0x74,0x75,0x72,0x65,0x28,0x30,0x29,0x5d,0x5d,0x2c,0x20,
    0x73,0x61,0x6d,0x70,0x6c,0x65,0x72,0x20,0x73,0x70,0x72,0x69,0x74,0x65,0x53,0x6d,
    0x70,0x6c,0x72,0x20,0x5b,0x5b,0x73,0x61,0x6d,0x70,0x6c,0x65,0x72,0x28,0x30,0x29,
    0x5d,0x5d,0x29,0x0a,0x7b,0x0a,0x20,0x20,0x20,0x20,0x6d,0x61,0x69,0x6e,0x30,0x5f,
    0x6f,0x75,0x74,0x20,0x6f,0x75,0x74,0x20,0x3d,0x20,0x7b,0x7d,0x3b,0x0a,0x23,0x6c,
    0x69,0x6e,0x65,0x20,0x31,0x33,0x20,0x22,0x61,0x73,0x73,0x65,0x74,0x73,0x2f,0x73,
    0x70,0x72,0x69,0x74,0x65,0x2e,0x67,0x6c,0x73,0x6c,0x22,0x0a,0x20,0x20,0x20,0x20,
    0x66,0x6c,0x6f,0x61,0x74,0x34,0x20,0x5f,0x32,0x31,0x20,0x3d,0x20,0x73,0x70,0x72,
    0x69,0x74,0x65,0x2e,0x73,0x61,0x6d,0x70,0x6c,0x65,0x28,0x73,0x70,0x72,0x69,0x74,
    0x65,0x53,0x6d,0x70,0x6c,0x72,0x2c,0x20,0x69,0x6e,0x2e,0x75,0x76,0x29,0x3b,0x0a,
    0x23,0x6c,0x69,0x6e,0x65,0x20,0x31,0x34,0x20,0x22,0x61,0x73,0x73,0x65,0x74,0x73,
    0x2f,0x73,0x70,0x72,0x69,0x74,0x65,0x2e,0x67,0x6c,0x73,0x6c,0x22,0x0a,0x20,0x20,
    0x20,0x20,0x66,0x6c,0x6f,0x61,0x74,0x34,0x20,0x5f,0x33,0x32,0x3b,0x0a,0x20,0x20,
    0x20,0x20,0x69,0x66,0x20,0x28,0x5f,0x32,0x31,0x2e,0x77,0x20,0x3e,0x20,0x30,0x2e,
    0x30,0x29,0x0a,0x20,0x20,0x20,0x20,0x7b,0x0a,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x5f,0x33,0x32,0x20,0x3d,0x20,0x73,0x65,0x6c,0x65,0x63,0x74,0x28,0x69,0x6e,
    0x2e,0x63,0x6f,0x6c,0x2c,0x20,0x5f,0x32,0x31,0x2c,0x20,0x62,0x6f,0x6f,0x6c,0x34,
    0x28,0x61,0x6c,0x6c,0x28,0x69,0x6e,0x2e,0x63,0x6f,0x6c,0x20,0x3d,0x3d,0x20,0x66,
    0x6c,0x6f,0x61,0x74,0x34,0x28,0x30,0x2e,0x30,0x2c,0x20,0x30,0x2e,0x30,0x2c,0x20,
    0x30,0x2e,0x30,0x2c,0x20,0x31,0x2e,0x30,0x29,0x29,0x29,0x29,0x3b,0x0a,0x20,0x20,
    0x20,0x20,0x7d,0x0a,0x20,0x20,0x20,0x20,0x65,0x6c,0x73,0x65,0x0a,0x20,0x20,0x20,
    0x20,0x7b,0x0a,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x5f,0x33,0x32,0x20,0x3d,
    0x20,0x66,0x6c,0x6f,0x61,0x74,0x34,0x28,0x30,0x2e,0x30,0x29,0x3b,0x0a,0x20,0x20,
    0x20,0x20,0x7d,0x0a,0x20,0x20,0x20,0x20,0x6f,0x75,0x74,0x2e,0x66,0x72,0x61,0x67,
    0x43,0x6f,0x6c,0x6f,0x72,0x20,0x3d,0x20,0x5f,0x33,0x32,0x3b,0x0a,0x20,0x20,0x20,
    0x20,0x72,0x65,0x74,0x75,0x72,0x6e,0x20,0x6f,0x75,0x74,0x3b,0x0a,0x7d,0x0a,0x0a,
    0x00,
};
#if !defined(SOKOL_GFX_INCLUDED)
  #error "Please include sokol_gfx.h before sprite.glsl.h"
#endif
static inline const sg_shader_desc* sprite_program_shader_desc(sg_backend backend) {
  if (backend == SG_BACKEND_METAL_MACOS) {
    static sg_shader_desc desc;
    static bool valid;
    if (!valid) {
      valid = true;
      desc.vs.source = sprite_vs_source_metal_macos;
      desc.vs.entry = "main0";
      desc.fs.source = sprite_fs_source_metal_macos;
      desc.fs.entry = "main0";
      desc.fs.images[0].name = "sprite";
      desc.fs.images[0].image_type = SG_IMAGETYPE_2D;
      desc.fs.images[0].sampler_type = SG_SAMPLERTYPE_FLOAT;
      desc.label = "sprite_program_shader";
    }
    return &desc;
  }
  return 0;
}