#include "MeshIslandsMaterial.h"

#include <cmath>

#include "PixieRendering/Renderer/IRenderer.h"

static const char* cVertexShaderSource = R"(
#version 450

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in ivec4 boneIDs; 
layout(location = 4) in vec4 boneWeights; 

layout(set = 0, binding = 0, std140) uniform CameraUBO {
    mat4 view;
    mat4 projection;
} camera;

layout(push_constant) uniform PushConstants {
    mat4 model;
	vec4 color;
} pc;

void main()
{
    gl_Position = camera.projection * camera.view * pc.model * vec4(aPos, 1.0);
}
)";

static const char* cFragmentShaderSource = R"(
#version 450

layout(location = 0) out vec4 FragColor;

layout(push_constant) uniform PushConstants {
    mat4 model;
	vec4 color;
} pc;

void main()
{
    FragColor = vec4(pc.color.rgb, 1.0);
}
)";

namespace PixieRenderer {

MeshIslandsMaterial::MeshIslandsMaterial() : IMaterial(cVertexShaderSource, cFragmentShaderSource) {
}

glm::vec4 MeshIslandsMaterial::GetColor() const {
	return m_color;
}

void MeshIslandsMaterial::SetColor(const glm::vec4& color) {
	m_color = color;
}

glm::vec4 MeshIslandsMaterial::MakeUniqueDebugColor(uint32_t id, float saturation, float value) {
	constexpr float kGoldenRatioConjugate = 0.6180339887498949f;

	const float h = std::fmod(static_cast<float>(id) * kGoldenRatioConjugate + 0.13f, 1.0f);
	const float s = saturation;
	const float v = value;

	const float c = v * s;
	const float hh = h * 6.0f;
	const float x = c * (1.0f - std::fabs(std::fmod(hh, 2.0f) - 1.0f));
	const float m = v - c;

	float r = 0.0f, g = 0.0f, b = 0.0f;
	if (hh < 1.0f) {
		r = c;
		g = x;
		b = 0.0f;
	} else if (hh < 2.0f) {
		r = x;
		g = c;
		b = 0.0f;
	} else if (hh < 3.0f) {
		r = 0.0f;
		g = c;
		b = x;
	} else if (hh < 4.0f) {
		r = 0.0f;
		g = x;
		b = c;
	} else if (hh < 5.0f) {
		r = x;
		g = 0.0f;
		b = c;
	} else {
		r = c;
		g = 0.0f;
		b = x;
	}

	return glm::vec4(r + m, g + m, b + m, 1.0f);
}

} // namespace PixieRenderer
