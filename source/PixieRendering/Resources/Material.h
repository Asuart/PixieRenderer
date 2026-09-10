#pragma once
#include <string>

namespace PixieRenderer {

class IRenderer;

struct Material {
	std::string name = "";
	const char* vertexShaderSource = nullptr;
	const char* fragmentShaderSource = nullptr;

	Material() = default;
	Material(const char* name, const char* vertShaderSource, const char* fragShaderSource)
	    : name(name), vertexShaderSource(vertShaderSource), fragmentShaderSource(fragShaderSource) {
	}
	Material(const std::string& name, const char* vertShaderSource, const char* fragShaderSource)
	    : name(name), vertexShaderSource(vertShaderSource), fragmentShaderSource(fragShaderSource) {
	}
	virtual void Bind(IRenderer*) {
	}
};

} // namespace PixieRenderer
