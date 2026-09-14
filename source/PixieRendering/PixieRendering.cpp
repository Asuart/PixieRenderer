#include "PixieRendering.h"

#include "Window/WindowOpenGL.h"
#include "Window/WindowVulkan.h"

namespace PixieRenderer {

IWindow* CreateWindow(std::string_view name, glm::uvec2 resolution, RenderAPI api) {
	switch (api) {
	case RenderAPI::OpenGL:
		return new WindowOpenGL(name, resolution);
	case RenderAPI::Vulkan:
		return new WindowVulkan(name, resolution);
	}
	return nullptr;
}

} // namespace PixieRenderer