#pragma once
#include "../IRenderer.h"

#include "PixieRenderer/ResourceManager/ResourceManagerOpenGL.h"

#include "ShaderCompilationOpenGL.h"
#include "ViewportStateOpenGL.h"

namespace PixieRenderer {

class IWindow;

class RendererOpenGL : public IRenderer {
  public:
	RendererOpenGL(IWindow* window);
	~RendererOpenGL();

	bool BeginFrame() override;
	void EndFrame() override;

	void BeginRenderPass(FrameBufferHandle handle = FrameBufferHandle()) override;
	void EndRenderPass() override;

	void SetRenderResolution(glm::uvec2 resolution) override;
	void SetViewport(ScreenRect rect) override;
	void SetScissor(ScreenRect rect) override;

	MeshHandle CreateMesh(const Mesh* mesh) override;
	void UpdateMesh(MeshHandle handle, const Mesh* mesh) override;

	FrameBufferHandle CreateFrameBuffer(glm::uvec2 resolution, TextureFormat format) override;
	glm::uvec2 GetFrameBufferResolution(FrameBufferHandle handle) override;
	void SetFrameBufferResolution(FrameBufferHandle handle, glm::uvec2 resolution) override;

	TextureHandle CreateTexture(const Image2D* image) override;
	void UpdateTexture(TextureHandle handle, const Image2D* image) override;
	glm::uvec2 GetTextureResolution(TextureHandle handle) override;
	void SetTextureFiltering(TextureHandle handle, TextureFiltering minFilter, TextureFiltering magFilter) override;
	void SetTextureWrap(TextureHandle handle, TextureWrap wrapU, TextureWrap wrapV, TextureWrap wrapW) override;

	BufferHandle CreateBuffer(BufferType type, size_t size) override;
	BufferHandle CreateBuffer(BufferType type, std::span<const std::byte> data) override;
	void UpdateBuffer(BufferHandle handle, std::span<const std::byte>, size_t offset = 0) override;
	size_t GetBufferSize(BufferHandle handle) override;
	std::vector<std::byte> ReadBuffer(BufferHandle handle, MemoryExtent extent) override;

	MaterialHandle CreateMaterial(const IMaterial* materialInfo) override;
	ComputeProgramHandle CreateComputeProgram(const IComputeProgram* computeInfo) override;

	void DrawMesh(DrawRequest request) override;
	void DispatchComputeProgram(DispatchRequest request) override;

	void BindTexture(
	    MaterialHandle materialHandle,
	    std::string_view name,
	    TextureHandle textureHandle,
	    uint32_t arrayIndex = 0
	) override;
	void BindTexture(
	    ComputeProgramHandle programHandle,
	    std::string_view name,
	    TextureHandle textureHandle,
	    uint32_t arrayIndex = 0
	) override;
	void BindTexture(
	    MaterialHandle materialHandle,
	    std::string_view name,
	    FrameBufferHandle frameBufferHandle,
	    uint32_t arrayIndex = 0
	) override;
	void BindTexture(
	    ComputeProgramHandle programHandle,
	    std::string_view name,
	    FrameBufferHandle frameBufferHandle,
	    uint32_t arrayIndex = 0
	) override;
	void BindBuffer(
	    MaterialHandle materialHandle,
	    std::string_view name,
	    BufferHandle bufferHandle,
	    MemoryExtent range = {}
	) override;
	void BindBuffer(
	    ComputeProgramHandle programHandle,
	    std::string_view name,
	    BufferHandle bufferHandle,
	    MemoryExtent range = {}
	) override;

	void WaitIdle() override;

	size_t GetUniformBufferOffsetAlignment() const override;
	size_t GetStorageBufferOffsetAlignment() const override;

	GLuint GetInternalTextureID(TextureHandle handle);
	GLuint GetInternalFrameBufferColorAttachmentID(FrameBufferHandle handle);

	void BeginStage(std::string_view /*name*/, StageType /*type*/) {

	}
	void EndStage() {

	}

	void UseResource(TextureHandle, ResourceUsage) { /* GL: glMemoryBarrier */
	}
	void UseResource(BufferHandle, ResourceUsage) { /* GL: glMemoryBarrier */
	}
	void UseResource(FrameBufferHandle, ResourceUsage) { /* GL: glMemoryBarrier */
	}

	void SetStageRenderTarget(FrameBufferHandle fbo) {
		BeginRenderPass(fbo);
	}
	void SetStageViewport(ScreenRect r) {
		SetViewport(r);
	}
	void SetStageScissor(ScreenRect r) {
		SetScissor(r);
	}

  private:
	glm::uvec2 m_surfaceResolution = { 0, 0 };
	ResourceManagerOpenGL m_resourceManager = {};
	std::vector<ViewportStateOpenGL> m_viewportStates;
	bool m_currentRenderPassOpen = false;

	void StoreViewportState();
	void RestoreViewportState();

	void GenerateTextureMipmaps(TextureHandle handle);
};

} // namespace PixieRenderer
