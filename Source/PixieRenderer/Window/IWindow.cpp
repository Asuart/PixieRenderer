#include "IWindow.h"
#include "PixieRenderer/pch.h"

#include "PixieRenderer/Renderer/IRenderer.h"

#include "WindowOpenGL.h"
#include "WindowVulkan.h"

namespace PixieRenderer {

std::unique_ptr<IWindow> IWindow::Create(const std::string& name, glm::uvec2 resolution, RenderAPI api) {
	switch (api) {
	case RenderAPI::OpenGL:
		return std::make_unique<WindowOpenGL>(name, resolution);
	case RenderAPI::Vulkan:
		return std::make_unique<WindowVulkan>(name, resolution);
	default:
		return nullptr;
	}
}

IWindow::IWindow(std::string_view name, glm::ivec2 resolution) : m_name(name), m_resolution(resolution) {
	if (!glfwInit()) {
		std::cerr << "Failed to Initialize GLFW\n";
		exit(1);
	}
}

IWindow::~IWindow() {
	glfwTerminate();
}

std::shared_ptr<IRenderer> IWindow::GetRenderer() const {
	return m_renderer;
}

glm::ivec2 IWindow::GetResolution() const {
	return m_resolution;
}

bool IWindow::GetShouldClose() const {
	return glfwWindowShouldClose(m_window);
}

GLFWwindow* IWindow::GetGLFWWindow() const {
	return m_window;
}

void IWindow::Close() {
	glfwSetWindowShouldClose(m_window, true);
}

void IWindow::SwapBuffers() {
	glfwSwapBuffers(m_window);
}

void IWindow::PollEvents() {
	glfwPollEvents();
}

void IWindow::OnResize(glm::uvec2 newSize) {
	if (m_resolution == newSize)
		return;
	m_resolution = newSize;
	if (m_renderer)
		m_renderer->SetRenderResolution(newSize);
}

void IWindow::SetDropCallback(DropCallback cb) {
	m_dropCallback = std::move(cb);
	if (m_window)
		glfwSetDropCallback(m_window, &IWindow::DropCallbackHandler);
}

void IWindow::DropCallbackHandler(GLFWwindow* w, int pathCount, const char** paths) {
	auto* self = static_cast<IWindow*>(glfwGetWindowUserPointer(w));
	if (!self || !self->m_dropCallback)
		return;

	std::vector<std::string> files;
	files.reserve(static_cast<size_t>(pathCount));
	for (int i = 0; i < pathCount; ++i) {
		if (paths[i])
			files.emplace_back(paths[i]);
	}
	self->m_dropCallback(files);
}

} // namespace PixieRenderer
