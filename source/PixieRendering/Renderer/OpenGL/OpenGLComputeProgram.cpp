#include "PixieRendering/pch.h"
#include "OpenGLComputeProgram.h"

namespace PixieRenderer {

OpenGLComputeProgram::OpenGLComputeProgram(GLuint program) : m_id(program) {
}

OpenGLComputeProgram::~OpenGLComputeProgram() {
	if (m_id) {
		glDeleteProgram(m_id);
		m_id = 0;
	}
}

OpenGLComputeProgram::OpenGLComputeProgram(OpenGLComputeProgram&& other) noexcept
    : m_id(other.m_id) {
	other.m_id = 0;
}

OpenGLComputeProgram& OpenGLComputeProgram::operator=(OpenGLComputeProgram&& other) noexcept {
	if (this != &other) {
		if (m_id)
			glDeleteProgram(m_id);
		m_id = other.m_id;
		other.m_id = 0;
	}
	return *this;
}

void OpenGLComputeProgram::Bind() {
	glUseProgram(m_id);
}

void OpenGLComputeProgram::BindTexture(const std::string& name, GLuint index) {
	GLint loc = glGetUniformLocation(m_id, name.c_str());
	if (loc != -1) {
		glUniform1i(loc, index);
	}
}

} // namespace PixieRenderer
