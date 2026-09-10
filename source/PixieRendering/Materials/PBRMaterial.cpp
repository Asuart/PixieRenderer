#include "PBRMaterial.h"

#include "PixieRendering/Renderer/IRenderer.h"

static const char* vertexShaderSource = R"(
#version 450

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in ivec4 boneIDs; 
layout(location = 4) in vec4 boneWeights; 

layout(location = 0) out vec2 TexCoord;

layout(set = 0, binding = 0, std140) uniform CameraUBO {
    mat4 view;
    mat4 projection;
} camera;

layout(set = 0, binding = 1, std140) uniform ModelUBO {
    mat4 model;
} modelData;

void main()
{
    gl_Position = camera.projection * camera.view * modelData.model * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
)";

static const char* fragmentShaderSource = R"(
#version 450

layout(location = 0) out vec4 FragColor;

layout(location = 0) in vec2 TexCoords;
layout(location = 1) in vec3 WorldPos;
layout(location = 2) in vec3 Normal;

layout(set = 0, binding = 2, std140) uniform CameraPosition {
    vec3 po;
} cameraPosition;

layout(set = 0, binding = 3, std140) uniform MaterialUBO {
    vec3 albedo;
    float metallic;
    float roughness;
} materialData;

layout(set = 0, binding = 4) uniform sampler2D texSampler;

float DistributionGGX(vec3 N, vec3 H, float a)
{
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float nom    = a2;
    float denom  = (NdotH2 * (a2 - 1.0) + 1.0);
    denom        = PI * denom * denom;
	
    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float k)
{
    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return nom / denom;
}
  
float GeometrySmith(vec3 N, vec3 V, vec3 L, float k)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = GeometrySchlickGGX(NdotV, k);
    float ggx2 = GeometrySchlickGGX(NdotL, k);
	
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

void main()
{
    vec3 N = normalize(Normal); 
    vec3 V = normalize(camPos - WorldPos);

    FragColor = texture(texSampler, TexCoord);
}
)";

namespace PixieRenderer {

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

void PBRMaterial::Bind(IRenderer* renderer) {
    struct PBRMaterialProps {
		glm::vec3 albedo;
		float metallic;
		float roughness;
    } props;
	props.albedo = m_albedo;
	props.metallic = m_metallic;
	props.roughness = m_roughness;
	renderer->LoadUniformBuffer(m_handle, "MaterialUBO", & props, sizeof(PBRMaterialProps));
}

} // namespace PixieRenderer
