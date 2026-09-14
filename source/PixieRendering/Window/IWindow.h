#pragma once
#include <string>
#include <string_view>

#include <glm/glm.hpp>

#include "PixieRendering/Renderer/RenderAPI.h"

struct GLFWwindow;

namespace PixieRenderer {

class IRenderer;

class IWindow {
  public:
	virtual ~IWindow();

	IRenderer* GetRenderer() const;
	glm::ivec2 GetResolution() const;
	bool GetShouldClose() const;
	RenderAPI GetRenderAPI() const;
	GLFWwindow* GetGLFWWindow() const;

	virtual void SwapBuffers();
	virtual void PollEvents();
	virtual void Close();

	virtual void OnResize(glm::uvec2 newSize);

  protected:
	IWindow(std::string_view name, glm::ivec2 resolution);

	std::string m_name = "Unnamed Window";
	GLFWwindow* m_window = nullptr;
	IRenderer* m_renderer = nullptr;
	glm::uvec2 m_resolution = { 0, 0 };
};

} // namespace PixieRenderer
