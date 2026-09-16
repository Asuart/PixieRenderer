#include "RendererOpenGL.h"
#include "PixieRenderer/pch.h"

#include "OpenGLCallbacks.h"

namespace PixieRenderer {

namespace {

GLenum ToOpenGLBufferTarget(BufferType type) {
	switch (type) {
	case BufferType::Uniform:
		return GL_UNIFORM_BUFFER;
	case BufferType::Storage:
		return GL_SHADER_STORAGE_BUFFER;
	case BufferType::Vertex:
		return GL_ARRAY_BUFFER;
	case BufferType::Index:
		return GL_ELEMENT_ARRAY_BUFFER;
	default:
		return GL_SHADER_STORAGE_BUFFER;
	}
}

} // namespace

RendererOpenGL::RendererOpenGL(IWindow*) {
	if (!gladLoadGL()) {
		std::cerr << "GLAD initialization failed\n";
		exit(2);
	}

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(OpenglCallbackHandler, 0);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

RendererOpenGL::~RendererOpenGL() {
}

bool RendererOpenGL::BeginFrame() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	return true;
}

void RendererOpenGL::EndFrame() {
	assert(m_viewportStates.empty() && "Unbalanced BindFrameBuffer/BindDefaultFrameBuffer");
}

void RendererOpenGL::BeginRenderPass(FrameBufferHandle handle) {
	if (m_currentRenderPassOpen) {
		EndRenderPass();
	}

	m_currentRenderPassOpen = true;

	if (handle) {
		OpenGLFrameBuffer& fb = m_resourceManager.GetFrameBuffer(handle);
		StoreViewportState();
		fb.Bind();
		fb.ResizeViewport();
	} else {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		RestoreViewportState();
	}
}

void RendererOpenGL::EndRenderPass() {
	RestoreViewportState();
	m_currentRenderPassOpen = false;
}

void RendererOpenGL::SetRenderResolution(glm::uvec2 /*resolution*/) {
}

void RendererOpenGL::SetViewport(ScreenRect rect) {
	glViewport(rect.origin.x, rect.origin.y, static_cast<GLsizei>(rect.size.x), static_cast<GLsizei>(rect.size.y));
}

void RendererOpenGL::SetScissor(ScreenRect rect) {
	glScissor(rect.origin.x, rect.origin.y, static_cast<GLsizei>(rect.size.x), static_cast<GLsizei>(rect.size.y));
}

MeshHandle RendererOpenGL::CreateMesh(const Mesh* mesh) {
	MeshHandle handle = m_resourceManager.CreateMesh();
	if (mesh != nullptr) {
		UpdateMesh(handle, mesh);
	}
	return handle;
}

void RendererOpenGL::UpdateMesh(MeshHandle handle, const Mesh* mesh) {
	OpenGLMesh& meshEntry = m_resourceManager.GetMesh(handle);
	meshEntry.Load(mesh);
}

FrameBufferHandle RendererOpenGL::CreateFrameBuffer(
    glm::uvec2 resolution,
    TextureFormat /*format*/
) {
	return m_resourceManager.CreateFrameBuffer(resolution);
}

glm::uvec2 RendererOpenGL::GetFrameBufferResolution(FrameBufferHandle handle) {
	OpenGLFrameBuffer& frameBufferEntry = m_resourceManager.GetFrameBuffer(handle);
	return frameBufferEntry.GetResolution();
}

void RendererOpenGL::SetFrameBufferResolution(FrameBufferHandle handle, glm::uvec2 resolution) {
	OpenGLFrameBuffer& frameBufferEntry = m_resourceManager.GetFrameBuffer(handle);
	frameBufferEntry.Resize(resolution);
}

TextureHandle RendererOpenGL::CreateTexture(const Image2D* image) {
	return m_resourceManager.CreateTexture(image);
}

void RendererOpenGL::UpdateTexture(TextureHandle handle, const Image2D* image) {
	OpenGLTexture& entry = m_resourceManager.GetTexture(handle);
	entry.Load(image);
}

glm::uvec2 RendererOpenGL::GetTextureResolution(TextureHandle handle) {
	const OpenGLTexture& texture = m_resourceManager.GetTexture(handle);
	return texture.GetResolution();
}

void RendererOpenGL::SetTextureFiltering(TextureHandle handle, TextureFiltering minFilter, TextureFiltering magFilter) {
	OpenGLTexture& texture = m_resourceManager.GetTexture(handle);
	texture.SetFiltering(CastTextureFilteringOpenGL(minFilter), CastTextureFilteringOpenGL(magFilter));
}

void RendererOpenGL::SetTextureWrap(TextureHandle handle, TextureWrap wrapU, TextureWrap wrapV, TextureWrap wrapW) {
	OpenGLTexture& texture = m_resourceManager.GetTexture(handle);
	texture.SetWrap(CastTextureWrapOpenGL(wrapU), CastTextureWrapOpenGL(wrapV), CastTextureWrapOpenGL(wrapW));
}

BufferHandle RendererOpenGL::CreateBuffer(BufferType type, size_t size) {
	GLenum target = ToOpenGLBufferTarget(type);
	BufferHandle handle = m_resourceManager.CreateBuffer(target);

	if (size != 0) {
		OpenGLBuffer& entry = m_resourceManager.GetBuffer(handle);
		entry.Load(nullptr, static_cast<GLuint>(size));
	}

	return handle;
}

BufferHandle RendererOpenGL::CreateBuffer(BufferType type, std::span<const std::byte> data) {
	GLenum target = ToOpenGLBufferTarget(type);
	BufferHandle handle = m_resourceManager.CreateBuffer(target);

	if (!data.empty()) {
		OpenGLBuffer& entry = m_resourceManager.GetBuffer(handle);
		entry.Load(reinterpret_cast<const uint8_t*>(data.data()), static_cast<GLuint>(data.size()));
	}

	return handle;
}

void RendererOpenGL::UpdateBuffer(BufferHandle handle, std::span<const std::byte> data, size_t offset) {
	OpenGLBuffer& entry = m_resourceManager.GetBuffer(handle);

	if (offset == 0 && data.size() == entry.GetSize()) {
		entry.Load(reinterpret_cast<const uint8_t*>(data.data()), static_cast<GLuint>(data.size()));
		return;
	}

	entry.Bind();
	glBufferSubData(entry.GetType(), static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(data.size()), data.data());
}

size_t RendererOpenGL::GetBufferSize(BufferHandle handle) {
	OpenGLBuffer& entry = m_resourceManager.GetBuffer(handle);
	return entry.GetSize();
}

std::vector<std::byte> RendererOpenGL::ReadBuffer(BufferHandle handle, MemoryExtent extent) {
	OpenGLBuffer& entry = m_resourceManager.GetBuffer(handle);
	entry.Bind();

	std::vector<std::byte> values(extent.size);
	glGetBufferSubData(
	    entry.GetType(),
	    static_cast<GLintptr>(extent.start),
	    static_cast<GLsizeiptr>(extent.size),
	    values.data()
	);
	return values;
}

MaterialHandle RendererOpenGL::CreateMaterial(const IMaterial* materialInfo) {
	return m_resourceManager.CreateMaterial(materialInfo);
}

ComputeProgramHandle RendererOpenGL::CreateComputeProgram(const IComputeProgram* computeInfo) {
	if (!computeInfo || !computeInfo->source) {
		return {};
	}

	GLuint program = CompileOpenGLComputeProgram(computeInfo->source);
	if (program == 0) {
		return {};
	}

	return m_resourceManager.CreateComputeProgram(program);
}

void RendererOpenGL::DrawMesh(DrawRequest request) {
	OpenGLGraphicsProgram& shaderEntry = m_resourceManager.GetMaterial(request.material);
	OpenGLMesh& meshEntry = m_resourceManager.GetMesh(request.mesh);

	shaderEntry.Bind();
	glBindVertexArray(meshEntry.GetVertexArrayObject());
	glDrawElements(GL_TRIANGLES, meshEntry.GetIndexCount(), GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);
	glUseProgram(0);
}

void RendererOpenGL::DispatchComputeProgram(DispatchRequest request) {
	OpenGLComputeProgram& computeShaderEntry = m_resourceManager.GetComputeProgram(request.program);
	computeShaderEntry.Bind();
	glDispatchCompute(static_cast<GLuint>(request.x), static_cast<GLuint>(request.y), static_cast<GLuint>(request.z));
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
	glUseProgram(0);
}

void RendererOpenGL::BindTexture(
    MaterialHandle materialHandle,
    std::string_view name,
    TextureHandle textureHandle,
    uint32_t arrayIndex
) {
	OpenGLGraphicsProgram& materialEntry = m_resourceManager.GetMaterial(materialHandle);
	OpenGLTexture& textureEntry = m_resourceManager.GetTexture(textureHandle);

	textureEntry.Bind(arrayIndex);
	materialEntry.BindTexture(std::string(name), arrayIndex);
}

void RendererOpenGL::BindTexture(
    ComputeProgramHandle computeMaterialHandle,
    std::string_view name,
    TextureHandle textureHandle,
    uint32_t arrayIndex
) {
	OpenGLComputeProgram& computeProgramEntry = m_resourceManager.GetComputeProgram(computeMaterialHandle);
	OpenGLTexture& textureEntry = m_resourceManager.GetTexture(textureHandle);

	textureEntry.BindImageTexture(arrayIndex);
	computeProgramEntry.BindTexture(std::string(name), arrayIndex);
}

void RendererOpenGL::BindBuffer(
    MaterialHandle materialHandle,
    std::string_view name,
    BufferHandle bufferHandle,
    MemoryExtent range
) {
	OpenGLGraphicsProgram& program = m_resourceManager.GetMaterial(materialHandle);
	OpenGLBuffer& buffer = m_resourceManager.GetBuffer(bufferHandle);

	const GLuint programId = program.GetID();
	const GLenum target = buffer.GetType();
	const std::string blockName(name);

	if (target == GL_UNIFORM_BUFFER) {
		GLuint blockIndex = program.GetUniformBlockIndex(blockName);
		if (blockIndex == GL_INVALID_INDEX) {
			return;
		}
		program.BindUniformBlock(blockName, blockIndex);
		buffer.Bind();
		if (range.size == 0) {
			glBindBufferBase(target, blockIndex, buffer.GetID());
		} else {
			glBindBufferRange(target, blockIndex, buffer.GetID(), range.start, range.size);
		}
		return;
	}

	if (target == GL_SHADER_STORAGE_BUFFER) {
		GLuint resourceIndex = glGetProgramResourceIndex(programId, GL_SHADER_STORAGE_BLOCK, blockName.c_str());
		if (resourceIndex == GL_INVALID_INDEX) {
			return;
		}
		glShaderStorageBlockBinding(programId, resourceIndex, resourceIndex);
		buffer.Bind();
		if (range.size == 0) {
			glBindBufferBase(target, resourceIndex, buffer.GetID());
		} else {
			glBindBufferRange(target, resourceIndex, buffer.GetID(), range.start, range.size);
		}
	}
}

void RendererOpenGL::BindBuffer(
    ComputeProgramHandle programHandle,
    std::string_view name,
    BufferHandle bufferHandle,
    MemoryExtent range
) {
	OpenGLComputeProgram& program = m_resourceManager.GetComputeProgram(programHandle);
	OpenGLBuffer& buffer = m_resourceManager.GetBuffer(bufferHandle);

	const GLuint programId = program.GetProgram();
	const GLenum target = buffer.GetType();
	const std::string blockName(name);

	if (target == GL_UNIFORM_BUFFER) {
		GLuint blockIndex = glGetUniformBlockIndex(programId, blockName.c_str());
		if (blockIndex == GL_INVALID_INDEX) {
			return;
		}
		glUniformBlockBinding(programId, blockIndex, blockIndex);
		buffer.Bind();
		if (range.size == 0) {
			glBindBufferBase(target, blockIndex, buffer.GetID());
		} else {
			glBindBufferRange(target, blockIndex, buffer.GetID(), range.start, range.size);
		}
		return;
	}

	if (target == GL_SHADER_STORAGE_BUFFER) {
		GLuint resourceIndex = glGetProgramResourceIndex(programId, GL_SHADER_STORAGE_BLOCK, blockName.c_str());
		if (resourceIndex == GL_INVALID_INDEX) {
			return;
		}
		glShaderStorageBlockBinding(programId, resourceIndex, resourceIndex);
		buffer.Bind();
		if (range.size == 0) {
			glBindBufferBase(target, resourceIndex, buffer.GetID());
		} else {
			glBindBufferRange(target, resourceIndex, buffer.GetID(), range.start, range.size);
		}
	}
}

void RendererOpenGL::WaitIdle() {
	glFinish();
}

size_t RendererOpenGL::GetUniformBufferOffsetAlignment() const {
	GLint alignment = 0;
	glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &alignment);
	return static_cast<size_t>(alignment);
}

size_t RendererOpenGL::GetStorageBufferOffsetAlignment() const {
	GLint alignment = 0;
	glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &alignment);
	return static_cast<size_t>(alignment);
}

GLuint RendererOpenGL::GetInternalTextureID(TextureHandle handle) {
	OpenGLTexture& textureEntry = m_resourceManager.GetTexture(handle);
	return textureEntry.GetID();
}

GLuint RendererOpenGL::GetInternalFrameBufferColorAttachmentID(FrameBufferHandle handle) {
	OpenGLFrameBuffer& fb = m_resourceManager.GetFrameBuffer(handle);
	return fb.GetColorAttachmentID();
}

void RendererOpenGL::StoreViewportState() {
	GLint originalViewport[4];
	glGetIntegerv(GL_VIEWPORT, originalViewport);

	ViewportStateOpenGL state;
	state.x = originalViewport[0];
	state.y = originalViewport[1];
	state.width = originalViewport[2];
	state.height = originalViewport[3];

	m_viewportStates.push_back(state);
}

void RendererOpenGL::RestoreViewportState() {
	if (m_viewportStates.empty()) {
		return;
	}
	ViewportStateOpenGL state = m_viewportStates.back();
	m_viewportStates.pop_back();
	glViewport(state.x, state.y, state.width, state.height);
}

void RendererOpenGL::GenerateTextureMipmaps(TextureHandle handle) {
	OpenGLTexture& texture = m_resourceManager.GetTexture(handle);
	texture.GenerateMipmaps();
}

} // namespace PixieRenderer
