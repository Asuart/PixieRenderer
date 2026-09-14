#include "OpenGLTexture.h"

namespace PixieRenderer {

OpenGLTexture::OpenGLTexture(const Image2D* image) {
	glGenTextures(1, &m_id);
	glBindTexture(GL_TEXTURE_2D, m_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, m_wrapS);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, m_wrapT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, m_wrapR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_minFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, m_magFilter);
	glBindTexture(GL_TEXTURE_2D, 0);

	if (image != nullptr) {
		Load(image);
	}
}

OpenGLTexture::~OpenGLTexture() {
	glDeleteTextures(1, &m_id);
}

GLuint OpenGLTexture::GetID() const {
	return m_id;
}

glm::uvec2 OpenGLTexture::GetResolution() const {
	return m_resolution;
}

void OpenGLTexture::Load(const Image2D* image) {
	if (image == nullptr) {
		return;
	}

	m_resolution = image->resolution;

	glBindTexture(GL_TEXTURE_2D, m_id);

	switch (image->format) {
	case TextureFormat::Red8:
		m_internalFormat = GL_RED;
		glTexImage2D(
		    GL_TEXTURE_2D,
		    0,
		    m_internalFormat,
		    image->resolution.x,
		    image->resolution.y,
		    0,
		    GL_RED,
		    GL_UNSIGNED_BYTE,
		    image->pixels.data()
		);
		break;
	case TextureFormat::RGBA8:
		m_internalFormat = GL_RGBA;
		glTexImage2D(
		    GL_TEXTURE_2D,
		    0,
		    m_internalFormat,
		    image->resolution.x,
		    image->resolution.y,
		    0,
		    GL_RGBA,
		    GL_UNSIGNED_BYTE,
		    image->pixels.data()
		);
		break;
	case TextureFormat::Red32f:
		m_internalFormat = GL_R32F;
		glTexImage2D(
		    GL_TEXTURE_2D,
		    0,
		    m_internalFormat,
		    image->resolution.x,
		    image->resolution.y,
		    0,
		    GL_RED,
		    GL_FLOAT,
		    image->pixels.data()
		);
		break;
	case TextureFormat::RGBA32f:
		m_internalFormat = GL_RGBA32F;
		glTexImage2D(
		    GL_TEXTURE_2D,
		    0,
		    m_internalFormat,
		    image->resolution.x,
		    image->resolution.y,
		    0,
		    GL_RGBA,
		    GL_FLOAT,
		    image->pixels.data()
		);
		break;
	default:
		throw "RendererOpenGL::CreateTexture: unhandled texture type";
	}

	glBindTexture(GL_TEXTURE_2D, 0);
}

void OpenGLTexture::Bind(uint32_t index) {
	glActiveTexture(static_cast<GLenum>(GL_TEXTURE0 + index));
	glBindTexture(GL_TEXTURE_2D, m_id);
}

void OpenGLTexture::BindImageTexture(uint32_t index) {
	glBindImageTexture(
	    static_cast<GLuint>(index),
	    m_id,
	    0,
	    GL_FALSE,
	    0,
	    GL_READ_WRITE,
	    m_internalFormat
	);
}

void OpenGLTexture::SetWrap(GLint wrapS, GLint wrapT, GLint wrapR) {
	m_wrapS = wrapS;
	m_wrapT = wrapT;
	m_wrapR = wrapR;

	glBindTexture(GL_TEXTURE_2D, m_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, wrapR);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void OpenGLTexture::SetFiltering(GLint minFilter, GLint magFilter) {
	m_minFilter = minFilter;
	m_magFilter = magFilter;

	glBindTexture(GL_TEXTURE_2D, m_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_minFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, m_magFilter);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void OpenGLTexture::GenerateMipmaps() {
	glBindTexture(GL_TEXTURE_2D, m_id);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace PixieRenderer
