#include "PixieRendering/pch.h"
#include "OpenGLMesh.h"

namespace PixieRenderer {

OpenGLMesh::OpenGLMesh() {
	glGenVertexArrays(1, &m_vertexArrayObject);
	glBindVertexArray(m_vertexArrayObject);

	glGenBuffers(1, &m_vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);

	glGenBuffers(1, &m_indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);

	glVertexAttribPointer(
	    0,
	    3,
	    GL_FLOAT,
	    GL_FALSE,
	    sizeof(Vertex),
	    (const void*)offsetof(Vertex, position)
	);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(
	    1,
	    3,
	    GL_FLOAT,
	    GL_FALSE,
	    sizeof(Vertex),
	    (const void*)offsetof(Vertex, normal)
	);
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(
	    2,
	    2,
	    GL_FLOAT,
	    GL_FALSE,
	    sizeof(Vertex),
	    (const void*)offsetof(Vertex, uv)
	);
	glEnableVertexAttribArray(2);

	glVertexAttribIPointer(
	    3,
	    Vertex::cBonesPerVertex,
	    GL_INT,
	    sizeof(Vertex),
	    (const void*)offsetof(Vertex, boneIDs)
	);
	glEnableVertexAttribArray(3);

	glVertexAttribPointer(
	    4,
	    Vertex::cBonesPerVertex,
	    GL_FLOAT,
	    GL_FALSE,
	    sizeof(Vertex),
	    (const void*)offsetof(Vertex, boneWeights)
	);
	glEnableVertexAttribArray(4);

	glBindVertexArray(0);
}

OpenGLMesh::~OpenGLMesh() {
	m_indexCount = 0;
	glDeleteBuffers(1, &m_vertexBuffer);
	m_vertexBuffer = 0;
	glDeleteBuffers(1, &m_indexBuffer);
	m_indexBuffer = 0;
	glDeleteVertexArrays(1, &m_vertexArrayObject);
	m_vertexArrayObject = 0;
}

GLuint OpenGLMesh::GetVertexArrayObject() const {
	return m_vertexArrayObject;
}

GLuint OpenGLMesh::GetIndexCount() const {
	return m_indexBuffer;
}

void OpenGLMesh::Load(const Mesh* mesh) {
	glBindVertexArray(m_vertexArrayObject);

	m_indexCount = static_cast<uint32_t>(mesh->indexes.size());

	if (mesh->indexes.size()) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
		glBufferData(
		    GL_ELEMENT_ARRAY_BUFFER,
		    sizeof(mesh->indexes[0]) * mesh->indexes.size(),
		    mesh->indexes.data(),
		    GL_STATIC_DRAW
		);
	}

	if (mesh->vertexes.size()) {
		glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
		glBufferData(
		    GL_ARRAY_BUFFER,
		    sizeof(mesh->vertexes[0]) * mesh->vertexes.size(),
		    mesh->vertexes.data(),
		    GL_STATIC_DRAW
		);
	}

	glBindVertexArray(0);
}

} // namespace PixieRenderer
