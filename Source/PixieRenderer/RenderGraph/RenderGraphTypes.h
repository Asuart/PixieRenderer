#pragma once
#include <cstdint>
#include <string>

#include <glm/glm.hpp>

#include "PixieRenderer/Buffer/BufferTypes.h"
#include "PixieRenderer/Image/ImageTypes.h"
#include "PixieRenderer/ResourceManager/ResourceHandles.h"

namespace PixieRenderer {

struct ScreenRect {
	glm::ivec2 origin = { 0, 0 };
	glm::uvec2 size = { 0, 0 };
};

struct MemoryExtent {
	size_t start = 0;
	size_t size = 0;
};

struct RGResource {
	uint32_t id = UINT32_MAX;

	inline bool IsValid() const {
		return id != UINT32_MAX;
	}

	inline bool operator==(const RGResource& o) const {
		return id == o.id;
	}
};

enum class ResourceUsage : uint8_t {
	Sampled,
	StorageRead,
	StorageWrite,
	UniformRead,
	VertexRead,
	IndexRead,
	IndirectRead,
	ColorAttachment,
	DepthAttachment,
	CopySrc,
	CopyDst,
	Present,
};

enum class StageType : uint8_t { Graphics, Compute, Transfer };

enum class ResourceKind : uint8_t {
	Texture,
	Buffer,
	RenderTarget,
	ImportedTexture,
	ImportedFrameBuffer,
	Present,
};

struct TextureDesc {
	TextureFormat format = TextureFormat::RGBA8;
	glm::uvec2 size = { 0, 0 };
	uint32_t mipLevels = 1;
	bool storageImage = false;
};

struct BufferDesc {
	BufferType type = BufferType::Storage;
	size_t size = 0;
};

struct RenderTargetDesc {
	TextureFormat format = TextureFormat::RGBA8;
	glm::uvec2 size = { 0, 0 };
	bool hasDepth = true;
	ResourceUsage finalUsage = ResourceUsage::Sampled;
};

} // namespace PixieRenderer
