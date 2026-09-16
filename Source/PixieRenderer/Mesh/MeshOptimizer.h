#pragma once
#include <memory>

#include "Mesh.h"
#include "MeshletMesh.h"

namespace PixieRenderer {

class MeshOptimizer {
  public:
	static std::shared_ptr<MeshletMesh> GenerateMeshletMesh(const std::shared_ptr<Mesh>& mesh);
};

} // namespace PixieRenderer
