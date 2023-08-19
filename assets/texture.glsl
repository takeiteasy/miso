@vs texture_vs
in vec2 position;
in vec2 texcoord;
in vec4 color;

out vec2 uv;
out vec4 col;

void main() {
    gl_Position = vec4(position.x, -position.y, 0.0, 1.0);
    uv = texcoord;
    col = color;
}
@end

@fs texture_fs
uniform sampler2D tex;
in vec2 uv;
in vec4 col;

out vec4 fragColor;

void main() {
    fragColor = texture(tex, uv) * col;
}
@end

@program texture_program texture_vs texture_fs
