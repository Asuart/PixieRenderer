#pragma once
#include <cstdint>
#include <vector>

#include "PixieRenderer/Math/SphereBounds.h"

namespace PixieRenderer {

struct Meshlet {
	uint32_t vertex_offset;
	uint32_t vertex_count;
	uint32_t triangle_offset;
	uint32_t triangle_count;
	SphereBounds bounds;
};

struct MeshletMesh {
	std::vector<Meshlet> meshlets;
	std::vector<uint32_t> meshlet_vertices;
	std::vector<uint8_t> meshlet_triangles;
};

} // namespace PixieRenderer
