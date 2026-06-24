#version 440

layout(location = 0) in vec2 vTexCoord;

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform BlurParameter{
    vec2 uTexelSize;
    vec2 uDirection;
    float uRadius;
};

layout(binding = 1) uniform sampler2D uTexture;

void main(){
    vec2 offset = uDirection * uTexelSize * uRadius;

    vec4 color = vec4(0.0);
    color += texture(uTexture, vTexCoord - offset * 4.0) * 0.05;
    color += texture(uTexture, vTexCoord - offset * 3.0) * 0.09;
    color += texture(uTexture, vTexCoord - offset * 2.0) * 0.12;
    color += texture(uTexture, vTexCoord - offset * 1.0) * 0.15;
    color += texture(uTexture, vTexCoord) * 0.18;
    color += texture(uTexture, vTexCoord + offset * 1.0) * 0.15;
    color += texture(uTexture, vTexCoord + offset * 2.0) * 0.12;
    color += texture(uTexture, vTexCoord + offset * 3.0) * 0.09;
    color += texture(uTexture, vTexCoord + offset * 4.0) * 0.05;

    fragColor = color;
}