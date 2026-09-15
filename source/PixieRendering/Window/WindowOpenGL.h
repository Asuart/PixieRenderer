#pragma once
#include "IWindow.h"
#include "PixieRendering/Renderer/RenderAPI.h"

namespace PixieRenderer {

class WindowOpenGL : public IWindow {
  public:
	WindowOpenGL(std::string_view name, glm::ivec2 resolution);
	~WindowOpenGL();

	RenderAPI GetRenderAPI() const override;
};

} // namespace PixieRenderer
