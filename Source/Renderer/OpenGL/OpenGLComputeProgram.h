#pragma once
#include <string>

#include <glad/glad.h>

namespace PixieRenderer {

struct OpenGLComputeProgram {
  public:
	OpenGLComputeProgram() = default;
	explicit OpenGLComputeProgram(GLuint program);
	~OpenGLComputeProgram();

	OpenGLComputeProgram(const OpenGLComputeProgram&) = delete;
	OpenGLComputeProgram& operator=(const OpenGLComputeProgram&) = delete;
	OpenGLComputeProgram(OpenGLComputeProgram&& other) noexcept;
	OpenGLComputeProgram& operator=(OpenGLComputeProgram&& other) noexcept;

	void Bind();
	void BindTexture(const std::string& name, GLuint index);
	GLuint GetProgram() const {
		return m_id;
	}

  private:
	GLuint m_id = 0;
};

} // namespace PixieRenderer
