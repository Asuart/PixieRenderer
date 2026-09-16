#pragma once
#include <cstdint>

namespace PixieRenderer {

enum class RenderAPI : uint32_t { Undefined = 0, OpenGL, Vulkan, COUNT };

} // namespace PixieRenderer
