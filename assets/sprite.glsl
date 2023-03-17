@vs sprite_vs
in vec2 position;
in vec2 texcoord;
in vec4 color;

out vec2 uv;
out vec4 hue;

void main() {
    gl_Position = vec4(position, 0.0, 1.0);
    uv = texcoord;
    hue = color;
}
@end

@fs sprite_fs
uniform sampler2D sprite;
in vec2 uv;
in vec4 hue;

out vec4 fragColor;

// Taken from http://lolengine.net/blog/2013/07/27/rgb-to-hsv-in-glsl. (WTFPL).

vec3 rgb2hsv(vec3 c) {
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
    vec4 color = texture(sprite, uv);
    vec3 rgb = color.rgb;
    vec3 hsv = rgb2hsv(rgb).xyz;
    hsv.x += hue.x;
    hsv.yz *= hue.yz;
    hsv.xyz = mod(hsv.xyz, 1.0);
    rgb = hsv2rgb(hsv);
    fragColor = vec4(rgb, color.w);
}
@end

@program sprite_program sprite_vs sprite_fs
