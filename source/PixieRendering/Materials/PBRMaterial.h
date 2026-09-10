#pragma once
#include "PixieRendering/Resources/Material.h"

#include <glm/glm.hpp>

#include "PixieRendering/Resources/ResourceHandles.h"

namespace PixieRenderer {

class PBRMaterial : public Material {
  public:
	glm::vec3 GetAlbedo() const;
	float GetMetallic() const;
	float GetRoughness() const;
	TextureHandle GetAlbedoTexture() const;
	TextureHandle GetNormalTexture() const;
	TextureHandle GetMetallicTexture() const;
	TextureHandle GetRoughnessTexture() const;

	void SetAlbedo(glm::vec3 albedo);
	void SetMetallic(float metallic);
	void SetRoughness(float roughness);
	void SetAlbedoTexture(TextureHandle texture);
	void SetNormalTexture(TextureHandle texture);
	void SetMetallicTexture(TextureHandle texture);
	void SetRoughnessTexture(TextureHandle texture);

	void Bind(IRenderer* renderer) override;

  private:
	MaterialHandle m_handle;
	glm::vec3 m_albedo = glm::vec3(1.0f, 1.0f, 1.0f);
	float m_metallic = 0.0f;
	float m_roughness = 1.0f;
	TextureHandle m_albedoTexture;
	TextureHandle m_normalTexture;
	TextureHandle m_metallicTexture;
	TextureHandle m_roughnessTexture;
};

} // namespace PixieRenderer
