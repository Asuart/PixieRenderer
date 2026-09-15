#pragma once
#include <cstdint>
#include <span>

#include "PixieRendering/ResourceManager/ResourceHandles.h"

namespace PixieRenderer {

static constexpr size_t cMaxRequestDataSize = 128;

struct DrawRequest {
	MaterialHandle material;
	MeshHandle mesh;
	std::span<const std::byte> inlineData{};
};

struct DispatchRequest {
	ComputeProgramHandle program;
	uint32_t x = 1;
	uint32_t y = 1;
	uint32_t z = 1;
	std::span<const std::byte> inlineData{};
};

} // namespace PixieRenderer
