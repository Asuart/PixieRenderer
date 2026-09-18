#include "PresentMaterial.h"

static const char* cPresentVS = R"(
#version 450

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in ivec4 boneIDs;
layout(location = 4) in vec4 boneWeights;

layout(location = 0) out vec2 vUV;

void main() {
    vUV = aPos.xy * 0.5 + 0.5;
    gl_Position = vec4(aPos.xy, 0.0, 1.0);
}
)";

static const char* cPresentFS = R"(
#version 450

layout(location = 0) in vec2 vUV;

layout(location = 0) out vec4 FragColor;

layout(set = 0, binding = 0) uniform sampler2D sceneTexture;

void main() { 
    FragColor = texture(sceneTexture, vUV);
}
)";

namespace PixieRenderer {

PresentMaterial::PresentMaterial() : IMaterial(cPresentVS, cPresentFS) {
}

} // namespace PixieRenderer
