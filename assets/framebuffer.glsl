@vs framebuffer_vs
in vec2 position;
in vec2 texcoord;

out vec2 uv;

void main(){
    gl_Position = vec4(position, 0.0, 1.0);
    uv = vec2(texcoord.x, -texcoord.y);
}
@end

@fs framebuffer_fs
uniform sampler2D tex;
in vec2 uv;

out vec4 frag_color;

void main(){
    frag_color = texture(tex, uv);
}
@end

@program framebuffer_program framebuffer_vs framebuffer_fs
