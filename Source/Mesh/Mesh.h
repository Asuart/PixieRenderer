#pragma once
#include <glm/glm.hpp>
#include <vector>

namespace PixieRenderer {

struct Vertex {
	static constexpr uint32_t cBonesPerVertex = 4;

	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 normal = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec2 uv = glm::vec2(0.0f);
	uint32_t boneIDs[cBonesPerVertex] = { 0 };
	float boneWeights[cBonesPerVertex] = { 0 };

	Vertex() = default;
	Vertex(
	    glm::vec3 _position,
	    glm::vec3 _normal = glm::vec3(0.0f, 0.0f, 1.0f),
	    glm::vec2 _uv = glm::vec2(0.0f)
	);
};

struct Mesh {
	std::vector<Vertex> vertexes;
	std::vector<uint32_t> indexes;

	Mesh() = default;

	inline uint64_t GetVertexBufferSize() const {
		return vertexes.size() * sizeof(Vertex);
	}

	inline uint64_t GetIndexBufferSize() const {
		return indexes.size() * sizeof(uint32_t);
	}
};

} // namespace PixieRenderer
