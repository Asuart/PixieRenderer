#pragma once
#include <memory>
#include <string_view>

#include <glm/glm.hpp>

#include "Renderer/RenderAPI.h"

namespace PixieRenderer {

class IWindow;

IWindow* CreateWindow(std::string_view name, glm::uvec2 resolution, RenderAPI api);

} // namespace PixieRenderer
