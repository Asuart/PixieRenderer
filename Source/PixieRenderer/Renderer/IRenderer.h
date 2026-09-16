#pragma once
#include <string_view>

#include "PixieRenderer/Buffer/BufferTypes.h"
#include "PixieRenderer/ComputeProgram/IComputeProgram.h"
#include "PixieRenderer/Image/Image2D.h"
#include "PixieRenderer/Material/IMaterial.h"
#include "PixieRenderer/Mesh/Mesh.h"
#include "PixieRenderer/RenderGraph/RenderGraphTypes.h"
#include "PixieRenderer/ResourceManager/ResourceHandles.h"
#include "Requests.h"

namespace PixieRenderer {

class IRenderer {
  public:
	virtual ~IRenderer() = default;

	virtual void SetPresentOverlayHook(std::function<void()> fn) {
	}
	virtual uint32_t GetSwapchainImageCount() const {
		return 3;
	}

	virtual bool BeginFrame() = 0;
	virtual void EndFrame() = 0;

	virtual void BeginRenderPass(FrameBufferHandle handle = FrameBufferHandle()) = 0;
	virtual void EndRenderPass() = 0;

	virtual void SetRenderResolution(glm::uvec2 resolution) = 0;
	virtual void SetViewport(ScreenRect rect) = 0;
	virtual void SetScissor(ScreenRect rect) = 0;

	virtual MeshHandle CreateMesh(const Mesh* mesh) = 0;
	virtual void UpdateMesh(MeshHandle handle, const Mesh* mesh) = 0;

	virtual FrameBufferHandle CreateFrameBuffer(glm::uvec2 resolution, TextureFormat format) = 0;
	virtual glm::uvec2 GetFrameBufferResolution(FrameBufferHandle handle) = 0;
	virtual void SetFrameBufferResolution(FrameBufferHandle handle, glm::uvec2 resolution) = 0;

	virtual TextureHandle CreateTexture(const Image2D* image) = 0;
	virtual void UpdateTexture(TextureHandle handle, const Image2D* image) = 0;
	virtual glm::uvec2 GetTextureResolution(TextureHandle handle) = 0;
	virtual void SetTextureFiltering(TextureHandle handle, TextureFiltering minFilter, TextureFiltering magFilter) = 0;
	virtual void SetTextureWrap(TextureHandle handle, TextureWrap wrapU, TextureWrap wrapV, TextureWrap wrapW) = 0;

	virtual BufferHandle CreateBuffer(BufferType type, size_t size) = 0;
	virtual BufferHandle CreateBuffer(BufferType type, std::span<const std::byte> data) = 0;
	virtual void UpdateBuffer(BufferHandle handle, std::span<const std::byte>, size_t offset = 0) = 0;
	virtual size_t GetBufferSize(BufferHandle handle) = 0;
	virtual std::vector<std::byte> ReadBuffer(BufferHandle handle, MemoryExtent extent) = 0;

	virtual MaterialHandle CreateMaterial(const IMaterial* materialInfo) = 0;
	virtual ComputeProgramHandle CreateComputeProgram(const IComputeProgram* computeInfo) = 0;

	virtual void DrawMesh(DrawRequest request) = 0;
	virtual void DispatchComputeProgram(DispatchRequest request) = 0;

	virtual void BindTexture(
	    MaterialHandle materialHandle,
	    std::string_view name,
	    TextureHandle textureHandle,
	    uint32_t arrayIndex = 0
	) = 0;
	virtual void BindTexture(
	    ComputeProgramHandle programHandle,
	    std::string_view name,
	    TextureHandle textureHandle,
	    uint32_t arrayIndex = 0
	) = 0;
	virtual void BindTexture(
	    MaterialHandle materialHandle,
	    std::string_view name,
	    FrameBufferHandle frameBufferHandle,
	    uint32_t arrayIndex = 0
	) = 0;
	virtual void BindTexture(
	    ComputeProgramHandle programHandle,
	    std::string_view name,
	    FrameBufferHandle frameBufferHandle,
	    uint32_t arrayIndex = 0
	) = 0;
	virtual void BindBuffer(
	    MaterialHandle materialHandle,
	    std::string_view name,
	    BufferHandle bufferHandle,
	    MemoryExtent range = {}
	) = 0;
	virtual void BindBuffer(
	    ComputeProgramHandle programHandle,
	    std::string_view name,
	    BufferHandle bufferHandle,
	    MemoryExtent range = {}
	) = 0;

	virtual void WaitIdle() = 0;

	virtual size_t GetUniformBufferOffsetAlignment() const = 0;
	virtual size_t GetStorageBufferOffsetAlignment() const = 0;

	// Used by stages
	virtual void BeginStage(std::string_view name, StageType type) = 0;
	virtual void EndStage() = 0;

	virtual void UseResource(TextureHandle h, ResourceUsage usage) = 0;
	virtual void UseResource(BufferHandle h, ResourceUsage usage) = 0;
	virtual void UseResource(FrameBufferHandle h, ResourceUsage usage) = 0;

	virtual void SetStageRenderTarget(FrameBufferHandle fbo) = 0;
	virtual void SetStageViewport(ScreenRect rect) = 0;
	virtual void SetStageScissor(ScreenRect rect) = 0;
};

} // namespace PixieRenderer
