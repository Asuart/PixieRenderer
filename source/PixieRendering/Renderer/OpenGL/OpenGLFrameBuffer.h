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

  protected:
	GLuint m_frameBuffer;
	GLuint m_texture;
	GLuint m_depth;
	glm::ivec2 m_resolution;
};

} // namespace PixieRenderer
