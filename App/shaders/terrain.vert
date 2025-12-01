#version 440 core

layout(location = 0) in vec2 aUV;

uniform sampler2D uHeight;
uniform mat4 uMVP;
uniform float uHeightScale;

out float vHeight;
out vec2 vUV;
out vec3 vWorldPos;

void main() {
    vec2 worldSize = vec2(100.0, 100.0); // taille fixe
    vUV = aUV;
    float h = texture(uHeight, aUV).r * uHeightScale;
    vHeight = h;
    vec2 posXZ = (aUV - vec2(0.5)) * worldSize;
    vWorldPos = vec3(posXZ.x, h, posXZ.y);
    gl_Position = uMVP * vec4(posXZ.x, h, posXZ.y, 1.0);
}

