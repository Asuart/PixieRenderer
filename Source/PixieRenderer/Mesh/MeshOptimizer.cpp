#include "PixieRenderer/pch.h"
#include "MeshOptimizer.h"

#include <meshoptimizer.h>

namespace PixieRenderer {

std::shared_ptr<MeshletMesh> MeshOptimizer::GenerateMeshletMesh(const std::shared_ptr<Mesh>& mesh) {
	const size_t vertex_count = mesh->vertexes.size();
	const size_t index_count = mesh->indexes.size();
	const float* vertex_positions = &mesh->vertexes[0].position.x;
	const size_t vertex_positions_stride = sizeof(Vertex);

	const size_t max_vertices = 64;
	const size_t max_triangles = 124;

	size_t max_meshlets = meshopt_buildMeshletsBound(index_count, max_vertices, max_triangles);

	std::vector<meshopt_Meshlet> meshlets(max_meshlets);
	std::vector<unsigned int> meshlet_vertices(index_count);
	std::vector<unsigned char> meshlet_triangles(index_count);

	size_t meshlet_count = meshopt_buildMeshlets(
	    meshlets.data(),
	    meshlet_vertices.data(),
	    meshlet_triangles.data(),
	    reinterpret_cast<const uint32_t*>(mesh->indexes.data()),
	    index_count,
	    vertex_positions,
	    vertex_count,
	    vertex_positions_stride,
	    max_vertices,
	    max_triangles,
	    0.0f
	);

	meshlets.resize(meshlet_count);
	meshlet_vertices.resize(meshlet_count * max_vertices);
	meshlet_triangles.resize(meshlet_count * max_triangles * 3);

	for (const meshopt_Meshlet& m : meshlets) {
		meshopt_optimizeMeshlet(
		    &meshlet_vertices[m.vertex_offset],
		    &meshlet_triangles[m.triangle_offset],
		    m.triangle_count,
		    m.vertex_count
		);
	}

	std::vector<meshopt_Bounds> bounds(meshlet_count);
	for (size_t i = 0; i < meshlet_count; ++i) {
		const meshopt_Meshlet& m = meshlets[i];
		bounds[i] = meshopt_computeMeshletBounds(
		    &meshlet_vertices[m.vertex_offset],
		    &meshlet_triangles[m.triangle_offset],
		    m.triangle_count,
		    vertex_positions,
		    vertex_count,
		    vertex_positions_stride
		);
	}

	return std::make_shared<MeshletMesh>();
}

} // namespace PixieRenderer
