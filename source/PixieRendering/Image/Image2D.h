#pragma once
#include <vector>
#include <glm/glm.hpp>

#include "ImageTypes.h"

namespace PixieRenderer {

struct Image2D {
	std::vector<uint8_t> pixels = {};
	glm::uvec2 resolution{0, 0};
	TextureFormat format = TextureFormat::Red8;
    TextureFiltering minFiltering = TextureFiltering::Linear;
    TextureFiltering magFiltering = TextureFiltering::Linear;
    TextureWrap wrapU = TextureWrap::Repeat;
    TextureWrap wrapV = TextureWrap::Repeat;
    TextureWrap wrapW = TextureWrap::Repeat;

    Image2D() = default;
    Image2D(TextureFormat _format);
	Image2D(TextureFormat _format, glm::uvec2 _resolution);

    void Clear();
	void Resize(glm::uvec2 _size);

    uint32_t GetPixelsCount() const;
	uint32_t GetByteSize() const;
};

} // namespace PixieRenderer
