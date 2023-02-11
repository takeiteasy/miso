@vs sprite_vs
in vec2 position;
in vec2 texcoord;
in vec4 color;

out vec2 uv;
out vec4 col;

void main() {
    gl_Position = vec4(position, 0.0, 1.0);
    uv = texcoord;
    col = color;
}
@end

@fs sprite_fs
uniform sampler2D sprite;
in vec2 uv;
in vec4 col;

out vec4 fragColor;

void main() {
    vec4 tex = texture(sprite, uv);
    fragColor = tex.a > 0 ? col == vec4(0.f, 0.f, 0.f, 1.f) ? tex : col : vec4(0.f);
}
@end

@program sprite_program sprite_vs sprite_fs
