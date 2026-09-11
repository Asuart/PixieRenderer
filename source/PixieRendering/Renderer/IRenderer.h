#pragma once
#include <string>

#include "../Window/Window.h"

#include "PixieRendering/RenderAPI.h"
#include "PixieRendering/Resources/Image2D.h"
#include "PixieRendering/Resources/Material.h"
#include "PixieRendering/Resources/Mesh.h"
#include "PixieRendering/Resources/ResourceHandles.h"
#include "PixieRendering/TextureEnums.h"

namespace PixieRenderer {

class Window;

class IRenderer {
  public:
	IRenderer(Window* window, RenderAPI renderAPI) : m_window(window), m_renderAPI(renderAPI) {
		window->SetRenderer(this);
	}

	virtual ~IRenderer() {};

	inline Window* GetWindow() const {
		return m_window;
	}

	inline RenderAPI GetRenderAPI() const {
		return m_renderAPI;
	}

	virtual bool BeginFrame() = 0;
	virtual void EndFrame() = 0;

	virtual void BeginRenderPass(FrameBufferHandle handle = FrameBufferHandle()) = 0;
	virtual void EndRenderPass() = 0;

	virtual void SetRenderResolution(glm::uvec2 resolution) = 0;
	virtual void SetViewport(glm::ivec2 start, glm::uvec2 resolution) = 0;
	virtual void SetScissor(glm::ivec2 start, glm::uvec2 resolution) = 0;

	virtual MeshHandle CreateMesh(const Mesh* mesh) = 0;
	virtual void LoadMesh(MeshHandle handle, const Mesh* mesh) = 0;
	virtual void DrawMesh(
	    MeshHandle meshHandle,
	    MaterialHandle materialHandle,
	    void* pushConstantsData = nullptr,
	    uint32_t pushConstantdsDataSize = 0
	) = 0;

	virtual FrameBufferHandle CreateFrameBuffer(
	    glm::uvec2 resolution,
	    TextureFormat format,
	    bool isPresentBuffer = false
	) = 0;
	virtual void ResizeFrameBuffer(FrameBufferHandle handle, glm::uvec2 resolution) = 0;
	virtual glm::uvec2 GetFrameBufferResolution(FrameBufferHandle handle) = 0;

	virtual TextureHandle CreateTexture(const Image2D* image) = 0;
	virtual void LoadTexture(TextureHandle handle, const Image2D* image) = 0;
	virtual void SetTextureFiltering(
	    TextureHandle handle,
	    TextureFiltering minFilter,
	    TextureFiltering magFilter
	) = 0;
	virtual void SetTextureWrap(
	    TextureHandle handle,
	    TextureWrap wrapU,
	    TextureWrap wrapV,
	    TextureWrap wrapW
	) = 0;
	virtual glm::ivec2 GetTextureResolution(TextureHandle handle) = 0;
	virtual void BindTexture(
	    MaterialHandle materialHandle,
	    const std::string& name,
	    TextureHandle textureHandle,
	    uint32_t index
	) = 0;
	virtual void BindTexture(
	    ComputeProgramHandle computeProgramHandle,
	    const std::string& name,
	    TextureHandle textureHandle,
	    uint32_t index
	) = 0;

	virtual ShaderStorageBufferHandle CreateShaderStorageBuffer(
	    const uint8_t* data,
	    uint32_t size
	) = 0;
	virtual void LoadShaderStorageBuffer(
	    ShaderStorageBufferHandle handle,
	    const uint8_t* data,
	    uint32_t size
	) = 0;
	virtual uint32_t GetShaderStorageBufferSize(ShaderStorageBufferHandle handle) = 0;
	virtual std::vector<uint8_t> GetShaderStorageBufferData(
	    ShaderStorageBufferHandle handle,
	    uint32_t offset,
	    uint32_t size
	) = 0;

	virtual UniformBufferHandle CreateUniformBuffer(const uint8_t* data, uint32_t size) = 0;
	virtual void LoadUniformBuffer(
	    UniformBufferHandle handle,
	    const uint8_t* data,
	    uint32_t size
	) = 0;
	virtual void LoadUniformBuffer(
	    MaterialHandle handle,
	    const std::string& name,
	    const void* data,
	    size_t size
	) = 0;

	virtual MaterialHandle CreateMaterial(const Material* materialInfo) = 0;

	virtual ComputeProgramHandle CreateComputeProgram(const char* source) = 0;
	virtual void DispatchComputeProgram(
	    ComputeProgramHandle handle,
	    int32_t x,
	    int32_t y,
	    int32_t z
	) = 0;

	virtual void WaitIdle() = 0;
	virtual void MemoryBarriersAll() = 0;

  protected:
	Window* m_window = nullptr;
	RenderAPI m_renderAPI = RenderAPI::Undefined;
	glm::uvec2 m_surfaceResolution = { 0, 0 };
};

} // namespace PixieRenderer
