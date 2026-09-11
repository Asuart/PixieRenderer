#include "RendererOpenGL.h"

#include <iostream>

#include "OpenGLCallbacks.h"

namespace PixieRenderer {

RendererOpenGL::RendererOpenGL(Window* mainWindow) : IRenderer(mainWindow, RenderAPI::OpenGL) {
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

void RendererOpenGL::SetRenderResolution(glm::uvec2 resolution) {
	glViewport(0, 0, resolution.x, resolution.y);
}

void RendererOpenGL::SetViewport(glm::ivec2 start, glm::uvec2 resolution) {
	glViewport(start.x, start.y, resolution.x, resolution.y);
}

void RendererOpenGL::SetScissor(glm::ivec2 start, glm::uvec2 resolution) {
	glScissor(start.x, start.y, resolution.x, resolution.y);
}

bool RendererOpenGL::BeginFrame() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	return true;
}

void RendererOpenGL::EndFrame() {
	assert(m_viewportStates.size() == 0);
}

void RendererOpenGL::BeginRenderPass(FrameBufferHandle handle) {
	if (handle) {
		OpenGLFrameBuffer& frameBufferEntry = m_resourceManager.GetFrameBufferEntry(handle);
		StoreViewportState();
		frameBufferEntry.Bind();
		frameBufferEntry.ResizeViewport();
	}
}

void RendererOpenGL::EndRenderPass() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	RestoreViewportState();
}

MeshHandle RendererOpenGL::CreateMesh(const Mesh* mesh) {
	MeshHandle handle = m_resourceManager.CreateMesh();

	if (mesh != nullptr) {
		LoadMesh(handle, mesh);
	}

	return handle;
}

void RendererOpenGL::LoadMesh(MeshHandle handle, const Mesh* mesh) {
	OpenGLMesh& meshEntry = m_resourceManager.GetMeshEntry(handle);
	meshEntry.Load(mesh);
}

void RendererOpenGL::DrawMesh(
    MeshHandle meshHandle,
    MaterialHandle materialHandle,
    void* /*pushConstantsData*/,
    uint32_t /*pushConstantdsDataSize*/
) {
	OpenGLGraphicsProgram& shaderEntry = m_resourceManager.GetMaterialEntry(materialHandle);
	OpenGLMesh& meshEntry = m_resourceManager.GetMeshEntry(meshHandle);
	shaderEntry.Bind();
	glBindVertexArray(meshEntry.GetVertexArrayObject());
	glDrawElements(GL_TRIANGLES, meshEntry.GetIndexCount(), GL_UNSIGNED_INT, NULL);
	glBindVertexArray(0);
	glUseProgram(0);

	// void RendererOpenGL::BindShaderStorageBuffer(ShaderStorageBufferHandle handle, uint32_t
	// index) {
	// 	ShaderStorageBufferOpenGL& entry = GetShaderStorageBufferEntry(handle);

	// 	glBindBuffer(GL_SHADER_STORAGE_BUFFER, entry.id);
	// 	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, entry.id);
	// }

	// void RendererOpenGL::BindUniformBuffer(UniformBufferHandle handle, uint32_t index) {
	// 	OpenGLUniformBuffer& entry = GetUniformBufferEntry(handle);

	// 	glBindBuffer(GL_UNIFORM_BUFFER, entry.id);
	// 	glBindBufferBase(GL_UNIFORM_BUFFER, index, entry.id);
	// }
}

FrameBufferHandle RendererOpenGL::CreateFrameBuffer(
    glm::uvec2 resolution,
    TextureFormat /*format*/,
    bool /*isPresent*/
) {
	return m_resourceManager.CreateFrameBuffer(resolution);
}

void RendererOpenGL::ResizeFrameBuffer(FrameBufferHandle handle, glm::uvec2 resolution) {
	OpenGLFrameBuffer& frameBufferEntry = m_resourceManager.GetFrameBufferEntry(handle);
	frameBufferEntry.Resize(resolution);
}

glm::uvec2 RendererOpenGL::GetFrameBufferResolution(FrameBufferHandle handle) {
	OpenGLFrameBuffer& frameBufferEntry = m_resourceManager.GetFrameBufferEntry(handle);
	return frameBufferEntry.GetResolution();
}

TextureHandle RendererOpenGL::CreateTexture(const Image2D* image) {
	return m_resourceManager.CreateTexture(image);
}

void RendererOpenGL::LoadTexture(TextureHandle handle, const Image2D* image) {
	OpenGLTexture& entry = m_resourceManager.GetTextureEntry(handle);
	entry.Load(image);
}

void RendererOpenGL::SetTextureFiltering(
    TextureHandle handle,
    TextureFiltering minFilter,
    TextureFiltering magFilter
) {
	OpenGLTexture& texture = m_resourceManager.GetTextureEntry(handle);
	texture
	    .SetFiltering(CastTextureFilteringOpenGL(minFilter), CastTextureFilteringOpenGL(magFilter));
}

void RendererOpenGL::SetTextureWrap(
    TextureHandle handle,
    TextureWrap wrapU,
    TextureWrap wrapV,
    TextureWrap wrapW
) {
	OpenGLTexture& texture = m_resourceManager.GetTextureEntry(handle);
	texture.SetWrap(
	    CastTextureWrapOpenGL(wrapU),
	    CastTextureWrapOpenGL(wrapV),
	    CastTextureWrapOpenGL(wrapW)
	);
}

void RendererOpenGL::GenerateTextureMipmaps(TextureHandle handle) {
	OpenGLTexture& texture = m_resourceManager.GetTextureEntry(handle);
	texture.GenerateMipmaps();
}

glm::ivec2 RendererOpenGL::GetTextureResolution(TextureHandle handle) {
	const OpenGLTexture& texture = m_resourceManager.GetTextureEntry(handle);
	return texture.GetResolution();
}

void RendererOpenGL::BindTexture(
    MaterialHandle materialHandle,
    const std::string& name,
    TextureHandle textureHandle,
    uint32_t index
) {
	OpenGLGraphicsProgram& materialEntry = m_resourceManager.GetMaterialEntry(materialHandle);
	OpenGLTexture& textureEntry = m_resourceManager.GetTextureEntry(textureHandle);
	textureEntry.Bind(index);
	materialEntry.BindTexture(name, index);
}

void RendererOpenGL::BindTexture(
    ComputeProgramHandle computeMaterialHandle,
    const std::string& name,
    TextureHandle textureHandle,
    uint32_t index
) {
	OpenGLComputeProgram& computeProgramEntry = m_resourceManager.GetComputeShaderEntry(
	    computeMaterialHandle
	);
	OpenGLTexture& textureEntry = m_resourceManager.GetTextureEntry(textureHandle);
	textureEntry.BindImageTexture(index);
	computeProgramEntry.BindTexture(name, index);
}

ShaderStorageBufferHandle RendererOpenGL::CreateShaderStorageBuffer(
    const uint8_t* data,
    uint32_t size
) {
	ShaderStorageBufferHandle handle = m_resourceManager.CreateShaderStorageBuffer(
	    GL_SHADER_STORAGE_BUFFER
	);

	if (data != nullptr && size != 0) {
		LoadShaderStorageBuffer(handle, data, size);
	}

	return handle;
}

void RendererOpenGL::LoadShaderStorageBuffer(
    ShaderStorageBufferHandle handle,
    const uint8_t* data,
    uint32_t size
) {
	OpenGLBuffer& entry = m_resourceManager.GetShaderStorageBufferEntry(handle);
	entry.Load(data, size);
}

uint32_t RendererOpenGL::GetShaderStorageBufferSize(ShaderStorageBufferHandle handle) {
	OpenGLBuffer& entry = m_resourceManager.GetShaderStorageBufferEntry(handle);
	return entry.GetSize();
}

std::vector<uint8_t> RendererOpenGL::GetShaderStorageBufferData(
    ShaderStorageBufferHandle handle,
    uint32_t offset,
    uint32_t size
) {
	OpenGLBuffer& entry = m_resourceManager.GetShaderStorageBufferEntry(handle);

	entry.Bind();

	std::vector<uint8_t> values(size);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, size, (GLvoid*)values.data());

	return values;
}

UniformBufferHandle RendererOpenGL::CreateUniformBuffer(const uint8_t* data, uint32_t size) {
	UniformBufferHandle handle = m_resourceManager.CreateUniformBuffer(GL_UNIFORM_BUFFER);

	if (data != nullptr && size != 0) {
		LoadUniformBuffer(handle, data, size);
	}

	return handle;
}

void RendererOpenGL::LoadUniformBuffer(
    UniformBufferHandle handle,
    const uint8_t* data,
    uint32_t size
) {
	OpenGLBuffer& buffer = m_resourceManager.GetUniformBufferEntry(handle);
	buffer.Load(data, size);
}

void RendererOpenGL::LoadUniformBuffer(
    MaterialHandle /*materialHandle*/,
    const std::string& /*name*/,
    const void* /*data*/,
    size_t /*size*/
) {
	// OpenGLGraphicsProgram& material = GetShaderEntry(materialHandle);
	// if (!material.nameToBindingMap.contains(name)) {
	//	std::cout << "LoadUniformBuffer: shader doesn't have binding '" << name << "'\n";
	//	return;
	// }
	// uint32_t binding = material.nameToBindingMap[name];
	// glBindBuffer(GL_UNIFORM_BUFFER, binding);
	// glBufferData(GL_UNIFORM_BUFFER, size, (GLvoid*)data, GL_DYNAMIC_DRAW);
}

MaterialHandle RendererOpenGL::CreateMaterial(const Material* materialInfo) {
	return m_resourceManager.CreateMaterial(materialInfo);
}

ComputeProgramHandle RendererOpenGL::CreateComputeProgram(const char* source) {
	if (!source) {
		return {};
	}

	GLuint program = CompileOpenGLComputeProgram(source);
	if (program == 0) {
		return {};
	}

	return m_resourceManager.CreateComputeProgram(program);
}

void RendererOpenGL::DispatchComputeProgram(
    ComputeProgramHandle handle,
    int32_t x,
    int32_t y,
    int32_t z
) {
	OpenGLComputeProgram& computeShaderEntry = m_resourceManager.GetComputeShaderEntry(handle);
	computeShaderEntry.Bind();
	glDispatchCompute(x, y, z);
	glUseProgram(0);
}

void RendererOpenGL::WaitIdle() {
	glFinish();
}

void RendererOpenGL::MemoryBarriersAll() {
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

GLuint RendererOpenGL::GetInternalTextureID(TextureHandle handle) {
	OpenGLTexture& textureEntry = m_resourceManager.GetTextureEntry(handle);
	return textureEntry.GetID();
}

GLuint RendererOpenGL::GetInternalFrameBufferColorAttachmentID(FrameBufferHandle handle) {
	OpenGLFrameBuffer& fb = m_resourceManager.GetFrameBufferEntry(handle);
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
	if (m_viewportStates.size() == 0) {
		return;
	}
	ViewportStateOpenGL state = m_viewportStates.back();
	m_viewportStates.pop_back();
	glViewport(state.x, state.y, state.width, state.height);
}

} // namespace PixieRenderer
