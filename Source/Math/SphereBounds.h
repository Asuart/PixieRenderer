#pragma once
#include <glm/glm.hpp>

namespace PixieRenderer {

struct SphereBounds {
	glm::vec3 center = glm::vec3(0.0f);
	float radius = 0.0f;

	SphereBounds() = default;
	SphereBounds(glm::vec3 _center, float _radius);
};

} // namespace PixieRenderer
