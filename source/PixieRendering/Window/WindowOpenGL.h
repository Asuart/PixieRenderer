#pragma once
#include "IWindow.h"

namespace PixieRenderer {

class WindowOpenGL : public IWindow {
  public:
	WindowOpenGL(std::string_view name, glm::ivec2 resolution);
	~WindowOpenGL();

  protected:
	void HandleEvent(const WindowEvent& event) override;
};

} // namespace PixieRenderer
