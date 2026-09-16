#pragma once
#include <cstdint>

namespace PixieRenderer {

struct ViewportStateOpenGL {
	int32_t x = 0;
	int32_t y = 0;
	int32_t width = 0;
	int32_t height = 0;

	ViewportStateOpenGL() = default;
};

} // namespace PixieRenderer
