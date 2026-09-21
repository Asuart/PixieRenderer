#include "WindowOpenGL.h"
#include "PixieRenderer/pch.h"

#include "PixieRenderer/Renderer/OpenGL/RendererOpenGL.h"

namespace PixieRenderer {

WindowOpenGL::WindowOpenGL(std::string_view name, glm::ivec2 resolution) : IWindow(name, resolution) {

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	m_window = glfwCreateWindow(resolution.x, resolution.y, name.data(), NULL, NULL);
	if (!m_window) {
		std::cerr << "Failed to create GLFW window\n";
		glfwTerminate();
		exit(1);
	}

	glfwMakeContextCurrent(m_window);

	glfwSetWindowUserPointer(m_window, this);
	glfwSetWindowSizeCallback(m_window, [](GLFWwindow* window, int width, int height) {
		IWindow* wnd = static_cast<IWindow*>(glfwGetWindowUserPointer(window));
		if (wnd) {
			wnd->OnResize(glm::ivec2(width, height));
		}
	});

	m_renderer = std::make_shared<RendererOpenGL>(this);
}

WindowOpenGL::~WindowOpenGL() {
	if (m_window) {
		glfwDestroyWindow(m_window);
	}
}

RenderAPI WindowOpenGL::GetRenderAPI() const {
	return RenderAPI::OpenGL;
}

} // namespace PixieRenderer
