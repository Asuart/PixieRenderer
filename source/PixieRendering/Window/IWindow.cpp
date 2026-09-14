#include "IWindow.h"

#include <iostream>

#include <GLFW/glfw3.h>

#include "PixieRendering/Renderer/IRenderer.h"

namespace PixieRenderer {

IWindow::IWindow(std::string_view name, glm::ivec2 resolution)
    : m_name(name), m_resolution(resolution) {
	if (!glfwInit()) {
		std::cerr << "Failed to Initialize GLFW\n";
		exit(1);
	}
}

IWindow::~IWindow() {
	glfwTerminate();
}

IRenderer* IWindow::GetRenderer() const {
	return m_renderer;
}

glm::ivec2 IWindow::GetResolution() const {
	return m_resolution;
}

bool IWindow::GetShouldClose() const {
	return glfwWindowShouldClose(m_window);
}

RenderAPI IWindow::GetRenderAPI() const {
	return m_renderer->GetRenderAPI();
}

GLFWwindow* IWindow::GetGLFWWindow() const {
	return m_window;
}

void IWindow::Close() {
	glfwSetWindowShouldClose(m_window, true);
}

void IWindow::HandleEvent(const WindowEvent&) {
}

void IWindow::SwapBuffers() {
	glfwSwapBuffers(m_window);
}

void IWindow::PollEvents() {
	glfwPollEvents();
}

void IWindow::OnResize(glm::uvec2 newSize) {
	if (m_resolution == newSize) {
		return;
	}
	m_resolution = newSize;
	if (m_renderer) {
		m_renderer->SetRenderResolution(newSize);
	}
}

} // namespace PixieRenderer
