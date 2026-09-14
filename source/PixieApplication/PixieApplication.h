#pragma once
#include <PixieRendering/PixieRendering.h>
#include <PixieRendering/Renderer/RenderAPI.h>
#include <PixieRendering/Renderer/IRenderer.h>
#include <PixieRendering/Window/IWindow.h>

namespace PixieApp {

class PixieApplication {
  public:
	PixieApplication(
	    const std::string& name,
	    glm::ivec2 resolution,
	    PixieRenderer::RenderAPI renderAPI
	);
	virtual ~PixieApplication() {
	}

	virtual void Start();

  protected:
	PixieRenderer::IRenderer* m_renderer;
	PixieRenderer::IWindow* m_window;

	virtual void OnStart() {
	}
	virtual void OnClose() {
	}
	virtual void BeforeDrawFrame() {
	}
	virtual void OnDrawFrame() {
	}
	virtual void AfterDrawFrame() {
	}
};

} // namespace PixieApp
