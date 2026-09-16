#pragma once
#include <glm/glm.hpp>

namespace PixieRenderer {

struct Camera {
	glm::mat4 view = glm::mat4(1.0f);
	glm::mat4 projection = glm::mat4(1.0f);

	Camera() = default;
};

} // namespace PixieRenderer
