#pragma once
#include <cstdint>

#include "ImageTypes.h"

namespace PixieRenderer {

constexpr uint32_t FormatToByteSize(TextureFormat format) {
	switch (format) {
	case TextureFormat::Red8:
		return 1;
	case TextureFormat::RGBA8:
		return 4;
	case TextureFormat::Red32f:
		return 4;
	case TextureFormat::RGBA32f:
		return 16;
	default:
		return 16;
	}
}

} // namespace PixieRenderer
