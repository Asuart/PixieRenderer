#include "PixieRenderer/pch.h"
#include "IMaterial.h"

namespace PixieRenderer {

IMaterial::IMaterial(std::string_view vertShaderSource, std::string_view fragShaderSource)
    : vertexShaderSource(vertShaderSource), fragmentShaderSource(fragShaderSource) {
}

MaterialHandle IMaterial::GetHandle() const {
	return m_handle;
}

void IMaterial::SetHandle(MaterialHandle handle) {
	m_handle = handle;
}

void IMaterial::Bind(IRenderer*) {
}

} // namespace PixieRenderer
