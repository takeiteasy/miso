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
    fragColor = texture(sprite, uv);
}
@end

@program sprite_program sprite_vs sprite_fs
