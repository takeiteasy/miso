@vs sprite_vs
in vec2 position;
in vec2 texcoord;

out vec2 uv;

void main() {
    gl_Position = vec4(position, 0.0, 1.0);
    uv = texcoord;
}
@end

@fs sprite_fs
uniform sampler2D sprite;
in vec2 uv;

out vec4 fragColor;

void main() {
    fragColor = texture(sprite, uv);
}
@end

@program sprite_program sprite_vs sprite_fs
