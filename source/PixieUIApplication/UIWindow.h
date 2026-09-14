#pragma once
#include <PixieRendering/Renderer/IRenderer.h>

namespace PixieUI {

class UI;

class UIWindow {
  public:
	UIWindow(UI* ui, PixieRenderer::IRenderer* renderer) : m_ui(ui), m_renderer(renderer) {};
	virtual ~UIWindow() {
	}

	virtual void OnBeforeDraw() {
	}
	virtual void Draw() = 0;

  protected:
	UI* m_ui;
	PixieRenderer::IRenderer* m_renderer;
};

} // namespace PixieUI
