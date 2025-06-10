#version 330 core
in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D yTex;
uniform sampler2D uTex;
uniform sampler2D vTex;

const mat3 yuv2rgb_bt601 = mat3(
            1.164,  1.164,  1.164,  // Y分量系数
            0.0,    -0.392, 2.017,  // U分量系数
            1.596,  -0.813, 0.0     // V分量系数
            );

void main() {
    float y = texture(yTex, TexCoord).r - 0.0625;  // Y ∈ [16/255, 235/255] → [0.063, 0.922]
    float u = texture(uTex, TexCoord).r - 0.5;     // U ∈ [16/255, 240/255] → [-0.5, 0.5]
    float v = texture(vTex, TexCoord).r - 0.5;     // V ∈ [16/255, 240/255] → [-0.5, 0.5]

    vec3 rgb = yuv2rgb_bt601 * vec3(y, u, v);
    FragColor = vec4(clamp(rgb, 0.0, 1.0), 1.0);
}
