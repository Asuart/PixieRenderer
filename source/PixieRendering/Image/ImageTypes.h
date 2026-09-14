#pragma once
#include <cstdint>

namespace PixieRenderer {

enum class TextureType : int32_t {
	Texture1D,
	Texture1DArray,
	Texture2D,
	Texture2DArray,
	Texture3D,
	Cubemap,
	CubemapArray
};

enum class TextureWrap : int32_t {
	Repeat,
	MirroredRepeat,
	ClampToEdge,
	ClampToBorder,
};

enum class TextureFiltering : int32_t {
	Nearest,
	Linear,
	NearestMipmapNearest,
	LinearMipmapNearest,
	NearestMipmapLinear,
	LinearMipmapLinear,
};

enum class TextureFormat : int32_t { Red8, RGBA8, Red32f, RGBA32f };

} // namespace PixieRenderer
