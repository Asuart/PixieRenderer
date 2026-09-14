#pragma once
#include "IWindow.h"

namespace PixieRenderer {

class WindowOpenGL : public IWindow {
  public:
	WindowOpenGL(std::string_view name, glm::ivec2 resolution);
	~WindowOpenGL();
};

} // namespace PixieRenderer
