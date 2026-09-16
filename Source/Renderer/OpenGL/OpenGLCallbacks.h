#pragma once
#include <iostream>

#include <glad/glad.h>

namespace PixieRenderer {

inline void APIENTRY OpenglCallbackHandler(
    GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar* message,
    const void* userParam
) {
	// silence warnings
	(void)userParam;

	if (id == 131185 || id == 131218 || id == 131186)
		return;

	std::cout << "---------------------opengl-callback-start------------\n";
	std::cout << "source: " << source << "\n";
	std::cout << "message length: " << length << "\n";
	std::cout << "message: " << message << "\n";
	std::cout << "type: ";
	switch (type) {
	case GL_DEBUG_TYPE_ERROR:
		std::cout << "ERROR";
		break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		std::cout << "DEPRECATED_BEHAVIOR";
		break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		std::cout << "UNDEFINED_BEHAVIOR";
		break;
	case GL_DEBUG_TYPE_PORTABILITY:
		std::cout << "PORTABILITY";
		break;
	case GL_DEBUG_TYPE_PERFORMANCE:
		std::cout << "PERFORMANCE";
		break;
	case GL_DEBUG_TYPE_OTHER:
		std::cout << "OTHER";
		break;
	default:
		std::cout << "UNDEFINED";
	}
	std::cout << "\n";

	std::cout << "id: " << id << "\n";
	std::cout << "severity: ";
	switch (severity) {
	case GL_DEBUG_SEVERITY_LOW:
		std::cout << "LOW";
		break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		std::cout << "MEDIUM";
		break;
	case GL_DEBUG_SEVERITY_HIGH:
		std::cout << "HIGH";
		break;
	default:
		std::cout << "UNDEFINED";
	}
	std::cout << "\n---------------------opengl-callback-end--------------\n";
}

} // namespace PixieRenderer
