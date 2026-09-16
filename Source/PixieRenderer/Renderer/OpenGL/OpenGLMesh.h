#pragma once
#include "glad/glad.h"

#include "PixieRenderer/Mesh/Mesh.h"

namespace PixieRenderer {

class OpenGLMesh {
  public:
	OpenGLMesh();
	~OpenGLMesh();

	GLuint GetVertexArrayObject() const;
	GLuint GetIndexCount() const;

	void Load(const Mesh* mesh);

  private:
	GLuint m_vertexArrayObject = 0;
	GLuint m_vertexBuffer = 0;
	GLuint m_indexBuffer = 0;
	GLuint m_indexCount = 0;
};

} // namespace PixieRenderer
