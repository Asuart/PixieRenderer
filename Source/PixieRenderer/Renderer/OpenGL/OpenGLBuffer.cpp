#include "OpenGLBuffer.h"
#include "PixieRenderer/pch.h"

namespace PixieRenderer {

OpenGLBuffer::OpenGLBuffer(GLenum type) : m_type(type) {
	glGenBuffers(1, &m_id);
}

OpenGLBuffer::~OpenGLBuffer() {
	glDeleteBuffers(1, &m_id);
}

GLuint OpenGLBuffer::GetID() const {
	return m_id;
}

GLuint OpenGLBuffer::GetSize() const {
	return m_size;
}

GLenum OpenGLBuffer::GetType() const {
	return m_type;
}

void OpenGLBuffer::Bind() {
	glBindBuffer(m_type, m_id);
}

void OpenGLBuffer::Load(const uint8_t* data, GLuint size) {
	m_size = size;
	glBindBuffer(m_type, m_id);
	glBufferData(m_type, size, (GLvoid*)data, GL_DYNAMIC_DRAW);
}

} // namespace PixieRenderer