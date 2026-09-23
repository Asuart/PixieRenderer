#pragma once
#include <cstdint>

namespace PixieRenderer {

enum class TextureType : uint32_t {
	Texture1D,
	Texture1DArray,
	Texture2D,
	Texture2DArray,
	Texture3D,
	Cubemap,
	CubemapArray
};

enum class TextureWrap : uint32_t {
	Repeat,
	MirroredRepeat,
	ClampToEdge,
	ClampToBorder,
};

enum class TextureFiltering : uint32_t {
	Nearest,
	Linear,
	NearestMipmapNearest,
	LinearMipmapNearest,
	NearestMipmapLinear,
	LinearMipmapLinear,
};

enum class TextureFormat : uint32_t { Red8, RGBA8, Red32f, RGBA32f };

} // namespace PixieRenderer
