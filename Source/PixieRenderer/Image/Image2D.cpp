#include "PixieRenderer/pch.h"
#include "Image2D.h"

#include "ImageUtils.h"

namespace PixieRenderer {

Image2D::Image2D(TextureFormat _format) : format(_format) {
}

Image2D::Image2D(TextureFormat _format, glm::uvec2 _resolution) : format(_format) {
	Resize(_resolution);
}

void Image2D::Clear() {
	for (size_t i = 0; i < pixels.size(); i++) {
		pixels[i] = 0;
	}
}

void Image2D::Resize(glm::uvec2 _resolution) {
	resolution = _resolution;
	pixels.resize(FormatToByteSize(format) * GetPixelsCount());
}

uint32_t Image2D::GetPixelsCount() const {
	return resolution.x * resolution.y;
}

uint32_t Image2D::GetByteSize() const {
	return static_cast<uint32_t>(pixels.size());
}

} // namespace PixieRenderer
