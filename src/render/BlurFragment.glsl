#version 330 core

in vec2 vTexCoord;
out vec4 fragColor;

uniform sampler2D uTexture;
uniform vec2 uTexelSize;
uniform float uRadius;

void main(){
    vec2 offset = uTexelSize * uRadius;

    vec4 color = vec4(0.0);
    color += texture(uTexture, vTexCoord + vec2(-4.0, 0.0) * offset) * 0.05;
    color += texture(uTexture, vTexCoord + vec2(-3.0, 0.0) * offset) * 0.09;
    color += texture(uTexture, vTexCoord + vec2(-2.0, 0.0) * offset) * 0.12;
    color += texture(uTexture, vTexCoord + vec2(-1.0, 0.0) * offset) * 0.15;
    color += texture(uTexture, vTexCoord) * 0.18;
    color += texture(uTexture, vTexCoord + vec2(1.0, 0.0) * offset) * 0.15;
    color += texture(uTexture, vTexCoord + vec2(2.0, 0.0) * offset) * 0.12;
    color += texture(uTexture, vTexCoord + vec2(3.0, 0.0) * offset) * 0.09;
    color += texture(uTexture, vTexCoord + vec2(4.0, 0.0) * offset) * 0.05;

    fragColor = color;
}