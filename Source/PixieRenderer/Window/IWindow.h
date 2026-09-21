#pragma once
#include <functional>
#include <string>
#include <string_view>
#include <vector>
#include <memory>

#include <glm/glm.hpp>

#include "PixieRenderer/Renderer/RenderAPI.h"

struct GLFWwindow;

namespace PixieRenderer {

class IRenderer;

class IWindow {
  public:
	using DropCallback = std::function<void(const std::vector<std::string>&)>;

	static std::unique_ptr<IWindow> Create(const std::string& name, glm::uvec2 resolution, RenderAPI api);

	virtual ~IWindow();

	std::shared_ptr<IRenderer> GetRenderer() const;
	glm::ivec2 GetResolution() const;
	bool GetShouldClose() const;
	GLFWwindow* GetGLFWWindow() const;

	virtual RenderAPI GetRenderAPI() const = 0;

	virtual void SwapBuffers();
	virtual void PollEvents();
	virtual void Close();

	virtual void OnResize(glm::uvec2 newSize);

	void SetDropCallback(DropCallback cb);

  protected:
	IWindow(std::string_view name, glm::ivec2 resolution);

	std::string m_name = "Unnamed Window";
	GLFWwindow* m_window = nullptr;
	std::shared_ptr<IRenderer> m_renderer = nullptr;
	glm::uvec2 m_resolution = { 0, 0 };
	DropCallback m_dropCallback;

  private:
	static void DropCallbackHandler(GLFWwindow* w, int pathCount, const char** paths);
};

} // namespace PixieRenderer
