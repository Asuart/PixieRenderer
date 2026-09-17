#pragma once
#include <glad/glad.h>

#include <glm/glm.hpp>

namespace PixieRenderer {

struct OpenGLFrameBuffer {
	OpenGLFrameBuffer(glm::ivec2 resolution);
	~OpenGLFrameBuffer();

	void Resize(glm::ivec2 resolution);
	glm::ivec2 GetResolution() const;
	GLuint GetBufferHandle() const;
	GLuint GetColorAttachmentID() const;
	GLuint GetDepthHandle() const;
	void ResizeViewport() const;
	void Clear() const;
	void Bind() const;
	void Unbind() const;
	void BindColorTexture(uint32_t index);
	void BindColorImageTexture(uint32_t index);

  protected:
	GLuint m_frameBuffer;
	GLuint m_texture;
	GLuint m_depth;
	GLint m_internalFormat = GL_RGBA;
	glm::ivec2 m_resolution;
};

} // namespace PixieRenderer
