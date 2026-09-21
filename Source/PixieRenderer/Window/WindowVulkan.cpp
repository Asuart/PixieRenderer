#include "PixieRenderer/pch.h"
#include "WindowVulkan.h"

#include "PixieRenderer/Renderer/Vulkan/RendererVulkan.h"

namespace PixieRenderer {

WindowVulkan::WindowVulkan(std::string_view name, glm::ivec2 resolution)
    : IWindow(name, resolution) {
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	m_window = glfwCreateWindow(resolution.x, resolution.y, name.data(), nullptr, nullptr);

	glfwSetWindowUserPointer(m_window, this);
	glfwSetWindowSizeCallback(m_window, [](GLFWwindow* window, int width, int height) {
		IWindow* wnd = static_cast<IWindow*>(glfwGetWindowUserPointer(window));
		if (wnd) {
			wnd->OnResize(glm::ivec2(width, height));
		}
	});

	m_renderer = std::make_shared<RendererVulkan>(this);
}

WindowVulkan::~WindowVulkan() {
	glfwDestroyWindow(m_window);
}

RenderAPI WindowVulkan::GetRenderAPI() const {
	return RenderAPI::Vulkan;
}

std::vector<const char*> WindowVulkan::GetRequiredExtensions() {
	uint32_t extensionCount = 0;
	const char** rawExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);

	std::vector<const char*> extensions(extensionCount);
	for (size_t i = 0; i < extensions.size(); i++) {
		extensions[i] = rawExtensions[i];
	}

	return extensions;
}

void WindowVulkan::CreateSurface(VkInstance vkInstance, VkSurfaceKHR& vkSurface) {
	if (glfwCreateWindowSurface(vkInstance, m_window, nullptr, &vkSurface) != VK_SUCCESS) {
		throw "failed to create window surface!";
	}
}

} // namespace PixieRenderer
