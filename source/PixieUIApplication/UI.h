#pragma once
#include <vector>

#include <PixieRendering/Renderer/IRenderer.h>

#include "UIImage.h"

namespace PixieRenderer {
class IWindow;
} // namespace PixieRenderer

namespace PixieUI {

class UIWindow;

class UI {
  public:
	UI(PixieRenderer::IWindow* window, bool docking);
	virtual ~UI();

	virtual void AddWindow(UIWindow* window);

	void OnBeforeDrawFrame();
	virtual void Draw() = 0;
	virtual UIImage* CreateUIImage(
	    PixieRenderer::IRenderer* renderer,
	    PixieRenderer::FrameBufferHandle handle
	) = 0;
	virtual UIImage* CreateUIImage(
	    PixieRenderer::IRenderer* renderer,
	    PixieRenderer::TextureHandle handle
	) = 0;

  protected:
	PixieRenderer::IWindow* m_window;
	std::vector<UIWindow*> m_windows;
	bool m_isDocking;

  public:
	template <typename T> std::vector<T*> GetWindowOfType() {
		std::vector<T*> result;
		for (size_t i = 0; i < m_windows.size(); i++) {
			T* cast = dynamic_cast<T*>(m_windows[i]);
			if (cast) {
				result.push_back(cast);
			}
		}
		return result;
	}
};

} // namespace PixieUI
