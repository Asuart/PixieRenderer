#include "OpenGLFrameBuffer.h"
#include "PixieRenderer/pch.h"

#include "PixieRenderer/LogCategories.h"

namespace {

std::string getFramebufferStatusString(GLenum status) {
	switch (status) {
	case GL_FRAMEBUFFER_UNDEFINED:
		return "GL_FRAMEBUFFER_UNDEFINED (The specified framebuffer is the default read or draw framebuffer, but the "
		       "default framebuffer does not exist)";
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT (One or more framebuffer attachment points are incomplete)";
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT (The framebuffer does not have at least one image "
		       "attached to it)";
	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
		return "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER (The value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE "
		       "for any color attachment point(s) named by GL_DRAW_BUFFERi)";
	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
		return "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER (GL_READ_BUFFER is assigned an attachment point that has no "
		       "image attached)";
	case GL_FRAMEBUFFER_UNSUPPORTED:
		return "GL_FRAMEBUFFER_UNSUPPORTED (The combination of internal formats of the attached images violates an "
		       "implementation-dependent set of restrictions)";
	case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
		return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE (The GL_TEXTURE_SAMPLES or GL_RENDERBUFFER_SAMPLES value is not "
		       "the same for all attached attachments)";
	case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
		return "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS (Any framebuffer attachment is layered, and any populated "
		       "attachment is not layered, or all populated attachments are not from textures of the same target)";
	case 0:
		return "An error occurred during status check (Returned 0, possibly context-related)";
	default:
		return "UNKNOWN_FRAMEBUFFER_STATUS (Code: " + std::to_string(status) + ")";
	}
}

} // namespace

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

	GLenum fbStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (fbStatus != GL_FRAMEBUFFER_COMPLETE) {
		Log::Error(LogCat::glFrameBuffer, "Failed to initialize FrameBuffer: {}", getFramebufferStatusString(fbStatus));
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
