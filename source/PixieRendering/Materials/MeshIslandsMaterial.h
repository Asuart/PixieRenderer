#pragma once

#include <cstdint>

#include <glm/glm.hpp>

#include "PixieRendering/Resources/Material.h"
#include "PixieRendering/Resources/ResourceHandles.h"

namespace PixieRenderer {

class MeshIslandsMaterial : public Material {
  public:
	MaterialHandle m_handle;

	MeshIslandsMaterial();

	glm::vec4 GetColor() const {
		return m_color;
	}

	void SetColor(const glm::vec4& color) {
		m_color = color;
	}

	void Bind(IRenderer* renderer) override;

	static glm::vec4 MakeUniqueDebugColor(uint32_t id, float saturation = 0.65f, float value = 0.90f);


  private:
	glm::vec4 m_color = glm::vec4(1.0f);
};

} // namespace PixieRenderer
