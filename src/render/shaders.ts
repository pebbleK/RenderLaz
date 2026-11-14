export const FULLSCREEN_VERTEX = `#version 300 es
layout(location = 0) in vec2 a_position;
out vec2 vTexCoord;

void main() {
  vTexCoord = a_position * 0.5 + 0.5;
  gl_Position = vec4(a_position, 0.0, 1.0);
}
`;

/**
 * 内置模糊着色器。四个 uniform 是渲染管线与着色器之间的唯一契约：
 * 改这段 GLSL 时保持它们的名称与类型不变。
 *   uTexelSize : vec2   1 / 画布尺寸
 *   uDirection : vec2   本 pass 的采样方向
 *   uRadius    : float  模糊半径（0.5–64.0）
 *   uTexture   : sampler2D 输入纹理
 */
export const DEFAULT_FRAGMENT = `#version 300 es
precision highp float;

in vec2 vTexCoord;
out vec4 fragColor;

uniform vec2 uTexelSize;
uniform vec2 uDirection;
uniform float uRadius;
uniform sampler2D uTexture;

const float W0 = 0.2270270270;
const float W1 = 0.1945945946;
const float W2 = 0.1216216216;
const float W3 = 0.0540540541;
const float W4 = 0.0162162162;

void main() {
  vec2 step = uDirection * uTexelSize * max(uRadius, 0.5) / 4.0;
  vec4 sum = texture(uTexture, vTexCoord) * W0;
  sum += texture(uTexture, vTexCoord + step * 1.0) * W1;
  sum += texture(uTexture, vTexCoord - step * 1.0) * W1;
  sum += texture(uTexture, vTexCoord + step * 2.0) * W2;
  sum += texture(uTexture, vTexCoord - step * 2.0) * W2;
  sum += texture(uTexture, vTexCoord + step * 3.0) * W3;
  sum += texture(uTexture, vTexCoord - step * 3.0) * W3;
  sum += texture(uTexture, vTexCoord + step * 4.0) * W4;
  sum += texture(uTexture, vTexCoord - step * 4.0) * W4;
  fragColor = vec4(sum.rgb, 1.0);
}
`;

export const BLUR_RADIUS_MIN = 0.5;
export const BLUR_RADIUS_MAX = 64;
