#include "Mesh.h"

namespace PixieRenderer {

Vertex::Vertex(glm::vec3 _position, glm::vec3 _normal, glm::vec2 _uv)
    : position(_position), normal(_normal), uv(_uv) {
}

} // namespace PixieRenderer
