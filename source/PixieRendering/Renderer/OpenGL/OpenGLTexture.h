#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>

#include "PixieRendering/Image/Image2D.h"

namespace PixieRenderer {

struct OpenGLTexture {
  public:
	OpenGLTexture(const Image2D* image);
	~OpenGLTexture();

	GLuint GetID() const;
	glm::uvec2 GetResolution() const;

	void Load(const Image2D* image);
	void Bind(uint32_t index);
	void BindImageTexture(uint32_t index);

	void SetWrap(GLint wrapS, GLint wrapT, GLint wrapR);
	void SetFiltering(GLint minFilter, GLint magFilter);
	void GenerateMipmaps();

  private:
	GLuint m_id = 0;
	GLint m_internalFormat = GL_RED;
	glm::ivec2 m_resolution = { 0, 0 };
	GLint m_wrapS = GL_CLAMP_TO_EDGE;
	GLint m_wrapT = GL_CLAMP_TO_EDGE;
	GLint m_wrapR = GL_CLAMP_TO_EDGE;
	GLint m_minFilter = GL_LINEAR;
	GLint m_magFilter = GL_LINEAR;
};

constexpr GLint CastTextureWrapOpenGL(TextureWrap wrap) {
	switch (wrap) {
	case TextureWrap::Repeat:
		return GL_REPEAT;
	case TextureWrap::MirroredRepeat:
		return GL_MIRRORED_REPEAT;
	case TextureWrap::ClampToEdge:
		return GL_CLAMP_TO_EDGE;
	case TextureWrap::ClampToBorder:
		return GL_CLAMP_TO_BORDER;
	default:
		return GL_REPEAT;
	}
}

constexpr GLint CastTextureFilteringOpenGL(TextureFiltering filtering) {
	switch (filtering) {
	case TextureFiltering::Linear:
		return GL_LINEAR;
	case TextureFiltering::Nearest:
		return GL_NEAREST;
	case TextureFiltering::NearestMipmapNearest:
		return GL_NEAREST_MIPMAP_NEAREST;
	case TextureFiltering::NearestMipmapLinear:
		return GL_NEAREST_MIPMAP_LINEAR;
	case TextureFiltering::LinearMipmapNearest:
		return GL_LINEAR_MIPMAP_NEAREST;
	case TextureFiltering::LinearMipmapLinear:
		return GL_LINEAR_MIPMAP_LINEAR;
	default:
		return GL_LINEAR;
	}
}

} // namespace PixieRenderer
