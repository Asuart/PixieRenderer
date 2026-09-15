#pragma once
#include "IMaterial.h"

#include <glm/glm.hpp>

namespace PixieRenderer {

class PBRMaterial : public IMaterial {
  public:
	PBRMaterial();

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
	glm::vec3 m_albedo = glm::vec3(1.0f, 1.0f, 1.0f);
	float m_metallic = 0.0f;
	float m_roughness = 1.0f;
	BufferHandle m_uniformBuffer;
	TextureHandle m_albedoTexture;
	TextureHandle m_normalTexture;
	TextureHandle m_metallicTexture;
	TextureHandle m_roughnessTexture;
};

} // namespace PixieRenderer
