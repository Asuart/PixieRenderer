#pragma once
#include "../IRenderer.h"

#include "PixieRendering/ResourceManager/ResourceManagerOpenGL.h"

#include "ShaderCompilationOpenGL.h"
#include "ViewportStateOpenGL.h"

namespace PixieRenderer {

class Window;

class RendererOpenGL : public IRenderer {
  public:
	RendererOpenGL(Window* mainWindow);
	~RendererOpenGL();

	bool BeginFrame() override;
	void EndFrame() override;

	void BeginRenderPass(FrameBufferHandle handle = FrameBufferHandle()) override;
	void EndRenderPass() override;

	void SetRenderResolution(glm::uvec2 resolution) override;
	void SetViewport(glm::ivec2 start, glm::uvec2 resolution) override;
	void SetScissor(glm::ivec2 start, glm::uvec2 resolution) override;

	MeshHandle CreateMesh(const Mesh* mesh) override;
	void LoadMesh(MeshHandle handle, const Mesh* mesh) override;
	void DrawMesh(
	    MeshHandle meshHandle,
	    MaterialHandle materialHandle,
	    void* pushConstantsData = nullptr,
	    uint32_t pushConstantdsDataSize = 0
	) override;

	FrameBufferHandle CreateFrameBuffer(glm::uvec2 resolution, TextureFormat format, bool) override;
	void ResizeFrameBuffer(FrameBufferHandle handle, glm::uvec2 resolution) override;
	glm::uvec2 GetFrameBufferResolution(FrameBufferHandle handle) override;

	TextureHandle CreateTexture(const Image2D* image) override;
	void LoadTexture(TextureHandle handle, const Image2D* image) override;
	void SetTextureFiltering(
	    TextureHandle handle,
	    TextureFiltering minFilter,
	    TextureFiltering magFilter
	) override;
	void SetTextureWrap(
	    TextureHandle handle,
	    TextureWrap wrapU,
	    TextureWrap wrapV,
	    TextureWrap wrapW
	) override;
	glm::ivec2 GetTextureResolution(TextureHandle handle) override;
	void BindTexture(
	    MaterialHandle materialHandle,
	    const std::string& name,
	    TextureHandle textureHandle,
	    uint32_t index
	) override;
	void BindTexture(
	    ComputeProgramHandle computeProgramHandle,
	    const std::string& name,
	    TextureHandle textureHandle,
	    uint32_t index
	) override;

	ShaderStorageBufferHandle
	CreateShaderStorageBuffer(const uint8_t* data, uint32_t size) override;
	void LoadShaderStorageBuffer(
	    ShaderStorageBufferHandle handle,
	    const uint8_t* data,
	    uint32_t size
	) override;
	uint32_t GetShaderStorageBufferSize(ShaderStorageBufferHandle handle) override;
	std::vector<uint8_t> GetShaderStorageBufferData(
	    ShaderStorageBufferHandle handle,
	    uint32_t offset,
	    uint32_t size
	) override;

	UniformBufferHandle CreateUniformBuffer(const uint8_t* data, uint32_t size) override;
	void LoadUniformBuffer(UniformBufferHandle handle, const uint8_t* data, uint32_t size) override;
	void LoadUniformBuffer(
	    MaterialHandle handle,
	    const std::string& name,
	    const void* data,
	    size_t size
	) override;

	MaterialHandle CreateMaterial(const Material* materialInfo) override;

	ComputeProgramHandle CreateComputeProgram(const char* source) override;
	void
	DispatchComputeProgram(ComputeProgramHandle handle, int32_t x, int32_t y, int32_t z) override;

	void WaitIdle() override;
	void MemoryBarriersAll() override;

	GLuint GetInternalTextureID(TextureHandle handle);
	GLuint GetInternalFrameBufferColorAttachmentID(FrameBufferHandle handle);

  private:
	ResourceManagerOpenGL m_resourceManager = {};
	std::vector<ViewportStateOpenGL> m_viewportStates;

	void StoreViewportState();
	void RestoreViewportState();

	void GenerateTextureMipmaps(TextureHandle handle);
};

} // namespace PixieRenderer
