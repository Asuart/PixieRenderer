#include "OpenGLFrameBuffer.h"
#include "PixieRenderer/pch.h"

namespace PixieRenderer {

OpenGLFrameBuffer::OpenGLFrameBuffer(glm::ivec2 resolution) : m_resolution(resolution) {
	glCreateFramebuffers(1, &m_frameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);

	glGenTextures(1, &m_texture);
	glBindTexture(GL_TEXTURE_2D, m_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, resolution.x, resolution.y, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture, 0);

	glGenTextures(1, &m_depth);
	glBindTexture(GL_TEXTURE_2D, m_depth);
	glTexImage2D(
	    GL_TEXTURE_2D,
	    0,
	    GL_DEPTH24_STENCIL8,
	    resolution.x,
	    resolution.y,
	    0,
	    GL_DEPTH_STENCIL,
	    GL_UNSIGNED_INT_24_8,
	    NULL
	);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_depth, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		throw "Failed to initializa FrameBuffer";
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

OpenGLFrameBuffer::~OpenGLFrameBuffer() {
	// glDeleteFramebuffers(1, &m_frameBuffer);
	// glDeleteTextures(1, &m_texture);
	// glDeleteTextures(1, &m_depth);
}

void OpenGLFrameBuffer::Resize(glm::ivec2 resolution) {
	if (m_resolution == resolution) {
		return;
	}
	m_resolution = resolution;
	glBindTexture(GL_TEXTURE_2D, m_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, resolution.x, resolution.y, 0, GL_RGBA, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, m_depth);
	glTexImage2D(
	    GL_TEXTURE_2D,
	    0,
	    GL_DEPTH24_STENCIL8,
	    resolution.x,
	    resolution.y,
	    0,
	    GL_DEPTH_STENCIL,
	    GL_UNSIGNED_INT_24_8,
	    NULL
	);
	glBindTexture(GL_TEXTURE_2D, 0);
}

glm::ivec2 OpenGLFrameBuffer::GetResolution() const {
	return m_resolution;
}

GLuint OpenGLFrameBuffer::GetBufferHandle() const {
	return m_frameBuffer;
}

GLuint OpenGLFrameBuffer::GetColorAttachmentID() const {
	return m_texture;
}

GLuint OpenGLFrameBuffer::GetDepthHandle() const {
	return m_depth;
}

void OpenGLFrameBuffer::ResizeViewport() const {
	glViewport(0, 0, m_resolution.x, m_resolution.y);
}

void OpenGLFrameBuffer::Clear() const {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGLFrameBuffer::Bind() const {
	glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);
}

void OpenGLFrameBuffer::Unbind() const {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGLFrameBuffer::BindColorTexture(uint32_t index) {
	glActiveTexture(static_cast<GLenum>(GL_TEXTURE0 + index));
	glBindTexture(GL_TEXTURE_2D, m_texture);
}

void OpenGLFrameBuffer::BindColorImageTexture(uint32_t index) {
	glBindImageTexture(static_cast<GLuint>(index), m_texture, 0, GL_FALSE, 0, GL_READ_WRITE, m_internalFormat);
}

} // namespace PixieRenderer
