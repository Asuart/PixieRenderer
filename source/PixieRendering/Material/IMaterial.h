#pragma once
#include <string>
#include <string_view>

#include "PixieRendering/ResourceManager/ResourceHandles.h"

namespace PixieRenderer {

class IRenderer;

class IMaterial {
  public:
	const std::string vertexShaderSource;
	const std::string fragmentShaderSource;

	IMaterial(std::string_view vertShaderSource, std::string_view fragShaderSource);

	MaterialHandle GetHandle() const;
	void SetHandle(MaterialHandle handle);

	virtual void Bind(IRenderer*);

  protected:
	MaterialHandle m_handle;
};

} // namespace PixieRenderer
