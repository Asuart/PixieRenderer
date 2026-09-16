#pragma once

#include <cstdint>

#include <glm/glm.hpp>

#include "IMaterial.h"
#include "PixieRenderer/ResourceManager/ResourceHandles.h"

namespace PixieRenderer {

class MeshIslandsMaterial : public IMaterial {
  public:

	MeshIslandsMaterial();

	glm::vec4 GetColor() const;
	void SetColor(const glm::vec4& color);

	static glm::vec4 MakeUniqueDebugColor(uint32_t id, float saturation = 0.65f, float value = 0.90f);

  private:
	glm::vec4 m_color = glm::vec4(1.0f);
};

} // namespace PixieRenderer
