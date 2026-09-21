#include "PixieRenderer/pch.h"
#include "PBRMaterial.h"

#include "PixieRenderer/Renderer/IRenderer.h"

static const char* cVertexShaderSource = R"(
#version 450

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in ivec4 boneIDs; 
layout(location = 4) in vec4 boneWeights; 

layout(location = 0) out vec2 TexCoord;
layout(location = 1) out vec3 WorldPos;
layout(location = 2) out vec3 Normal;

layout(set = 0, binding = 0, std140) uniform CameraUBO {
    mat4 view;
    mat4 projection;
} camera;

layout(push_constant) uniform PushConstants {
    mat4 model;
} pc;

void main()
{
    vec4 worldPos = pc.model * vec4(aPos, 1.0);
    WorldPos = worldPos.xyz;
    Normal   = mat3(pc.model) * aNormal;
    TexCoord = aTexCoord;
    gl_Position = camera.projection * camera.view * worldPos;
}
)";

static const char* cFragmentShaderSource = R"(
#version 450

layout(location = 0) in vec2 TexCoord;
layout(location = 1) in vec3 WorldPos;
layout(location = 2) in vec3 Normal;

layout(location = 0) out vec4 FragColor;

layout(set = 0, binding = 2, std140) uniform CameraPosition {
    vec4 po;
} cameraPosition;

layout(set = 0, binding = 3, std140) uniform MaterialUBO {
    vec4  albedo;
    float metallic;
    float roughness;
    vec2  _pad;
} materialData;

layout(set = 0, binding = 4) uniform sampler2D albedoTexture;
layout(set = 0, binding = 5) uniform sampler2D metallicTexture;
layout(set = 0, binding = 6) uniform sampler2D roughnessTexture;
layout(set = 0, binding = 7) uniform sampler2D normalTexture;

const float PI = 3.14159265359;

const vec3 LightDirection = normalize(vec3(-0.5, -1.0, -0.3));
const vec3 LightColor     = vec3(3.0);

float DistributionGGX(vec3 N, vec3 H, float a)
{
    float a2    = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / max(denom, 1e-6);
}

float GeometrySchlickGGX(float NdotV, float k)
{
    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return nom / max(denom, 1e-6);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float k)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(NdotV, k) * GeometrySchlickGGX(NdotL, k);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main()
{
    vec3 albedo = texture(albedoTexture, TexCoord).rgb * materialData.albedo.rgb;
    float metallic  = texture(metallicTexture, TexCoord).r * materialData.metallic;
    float roughness = texture(roughnessTexture, TexCoord).r * materialData.roughness;

    vec3 N = normalize(Normal);

    vec3 normalMap = texture(normalTexture, TexCoord).rgb;
    normalMap = normalMap * 2.0 - 1.0;

    vec3 dp1 = dFdx(WorldPos);
    vec3 dp2 = dFdy(WorldPos);
    vec2 duv1 = dFdx(TexCoord);
    vec2 duv2 = dFdy(TexCoord);

    vec3 dp2perp = cross(dp2, N);
    vec3 dp1perp = cross(N, dp1);

    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;

    float invmax = inversesqrt(max(dot(T, T), dot(B, B)));
    mat3 TBN = mat3(T * invmax, B * invmax, N);

    N = normalize(TBN * normalMap);

    vec3 V = normalize(cameraPosition.po.xyz - WorldPos);
    vec3 L = -LightDirection;
    vec3 H = normalize(V + L);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    float a   = roughness * roughness;
    float NDF = DistributionGGX(N, H, a);
    float k   = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    float G   = GeometrySmith(N, V, L, k);
    vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator   = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 1e-4;
    vec3 specular = numerator / denominator;

    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);

    float NdotL = max(dot(N, L), 0.0);
    vec3  Lo    = (kD * albedo / PI + specular) * LightColor * NdotL;

    vec3 ambient = vec3(0.03) * albedo;

    vec3 color = ambient + Lo;
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}
)";

namespace PixieRenderer {

PBRMaterial::PBRMaterial() : IMaterial(cVertexShaderSource, cFragmentShaderSource) {
}

glm::vec3 PBRMaterial::GetAlbedo() const {
	return m_albedo;
}

float PBRMaterial::GetMetallic() const {
	return m_metallic;
}

float PBRMaterial::GetRoughness() const {
	return m_roughness;
}

TextureHandle PBRMaterial::GetAlbedoTexture() const {
	return m_albedoTexture;
}

TextureHandle PBRMaterial::GetNormalTexture() const {
	return m_normalTexture;
}

TextureHandle PBRMaterial::GetMetallicTexture() const {
	return m_metallicTexture;
}

TextureHandle PBRMaterial::GetRoughnessTexture() const {
	return m_roughnessTexture;
}

void PBRMaterial::SetAlbedo(glm::vec3 albedo) {
	m_albedo = albedo;
}

void PBRMaterial::SetMetallic(float metallic) {
	m_metallic = metallic;
}

void PBRMaterial::SetRoughness(float roughness) {
	m_roughness = roughness;
}

void PBRMaterial::SetAlbedoTexture(TextureHandle texture) {
	m_albedoTexture = texture;
}

void PBRMaterial::SetNormalTexture(TextureHandle texture) {
	m_normalTexture = texture;
}

void PBRMaterial::SetMetallicTexture(TextureHandle texture) {
	m_metallicTexture = texture;
}

void PBRMaterial::SetRoughnessTexture(TextureHandle texture) {
	m_roughnessTexture = texture;
}

void PBRMaterial::Bind(std::shared_ptr<IRenderer> renderer) {
	struct PBRMaterialProps {
		glm::vec4 albedo;
		float metallic;
		float roughness;
		glm::vec2 _pad;
	} props{};
	props.albedo = glm::vec4(m_albedo, 0.0f);
	props.metallic = m_metallic;
	props.roughness = m_roughness;

	const auto bytes = std::as_bytes(std::span{ &props, 1 });

	if (!m_uniformBuffer) {
		m_uniformBuffer = renderer->CreateBuffer(BufferType::Uniform, bytes);
	} else {
		renderer->UpdateBuffer(m_uniformBuffer, bytes);
	}
	renderer->BindBuffer(m_handle, "materialData", m_uniformBuffer);

	if (m_albedoTexture)
		renderer->BindTexture(m_handle, "albedoTexture", m_albedoTexture);
	if (m_metallicTexture)
		renderer->BindTexture(m_handle, "metallicTexture", m_metallicTexture);
	if (m_roughnessTexture)
		renderer->BindTexture(m_handle, "roughnessTexture", m_roughnessTexture);
	if (m_normalTexture)
		renderer->BindTexture(m_handle, "normalTexture", m_normalTexture);
}

} // namespace PixieRenderer
