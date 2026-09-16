#pragma once
#include <string>
#include <unordered_map>

#include <glad/glad.h>

#include "PixieRendering/Material/IMaterial.h"

namespace PixieRenderer {

struct OpenGLGraphicsProgram {
  public:
	explicit OpenGLGraphicsProgram(const IMaterial* materialInfo);
	~OpenGLGraphicsProgram();

	OpenGLGraphicsProgram(const OpenGLGraphicsProgram&) = delete;
	OpenGLGraphicsProgram& operator=(const OpenGLGraphicsProgram&) = delete;
	OpenGLGraphicsProgram(OpenGLGraphicsProgram&& other) noexcept;
	OpenGLGraphicsProgram& operator=(OpenGLGraphicsProgram&& other) noexcept;

	GLuint GetID() const;

	void Bind();
	void BindTexture(const std::string& name, GLuint index);

	void BindUniformBlock(const std::string& name, GLuint bindingPoint);
	GLuint GetUniformBlockIndex(const std::string& name) const;

  private:
	GLuint m_id = 0;
	mutable std::unordered_map<std::string, GLuint> m_uniformBlockCache;
};

} // namespace PixieRenderer
