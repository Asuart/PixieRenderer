#pragma once
#include <cstdint>

#include <glad/glad.h>

namespace PixieRenderer {

struct OpenGLBuffer {
  public:
	OpenGLBuffer(GLenum type);
	~OpenGLBuffer();

	GLuint GetID() const;
	GLuint GetSize() const;
	GLenum GetType() const;

	void Bind();

	void Load(const uint8_t* data, GLuint size);

  private:
	GLuint m_id = 0;
	GLuint m_size = 0;
	GLenum m_type = GL_INVALID_ENUM;
};

} // namespace PixieRenderer