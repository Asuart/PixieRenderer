#pragma once
#include "IWindow.h"

#include <vulkan/vulkan.h>

namespace PixieRenderer {

class RendererVulkan;

class WindowVulkan : public IWindow {
  public:
	WindowVulkan(std::string_view name, glm::ivec2 resolution);
	~WindowVulkan();

	std::vector<const char*> GetRequiredExtensions();
	void CreateSurface(VkInstance vkInstance, VkSurfaceKHR& vkSurface);

	RendererVulkan* GetRendererVulkan();
};

} // namespace PixieRenderer
