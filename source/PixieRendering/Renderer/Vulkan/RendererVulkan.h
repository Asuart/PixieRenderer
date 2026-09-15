#pragma once
#include "../IRenderer.h"

#include <vector>

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "VulkanDevice.h"
#include "VulkanInstance.h"
#include "VulkanSwapchain.h"
#include "PixieRendering/ResourceManager/ResourceManagerVulkan.h"
#include "PixieRendering/Material/IMaterial.h"

namespace PixieRenderer {

class RendererVulkan : public IRenderer {
  public:
	RendererVulkan(IWindow* window);

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

	FrameBufferHandle CreateFrameBuffer(glm::uvec2 resolution, TextureFormat format, bool isPresent)
	    override;
	void ResizeFrameBuffer(FrameBufferHandle handle, glm::uvec2 resolution) override;
	glm::uvec2 GetFrameBufferResolution(FrameBufferHandle handle) override;

	TextureHandle CreateTexture(const Image2D* image, uint32_t mipLevels = 1) override;
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

	ShaderStorageBufferHandle CreateShaderStorageBuffer(const uint8_t* data, uint32_t size)
	    override;
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

	MaterialHandle CreateMaterial(const IMaterial* material) override;

	ComputeProgramHandle CreateComputeProgram(const char* source) override;
	void DispatchComputeProgram(ComputeProgramHandle handle, int32_t x, int32_t y, int32_t z)
	    override;

	void WaitIdle() override;
	void MemoryBarriersAll() override;

  public:
	VkInstance GetInstance() const;
	VkPhysicalDevice GetPhysicalDevice() const;
	VkDevice GetDevice() const;
	VkQueue GetGraphicsQueue() const;
	VkQueue GetPresentQueue() const;
	VkRenderPass GetPresentRenderPass() const;
	VkCommandBuffer GetCurrentFrameCommandBuffer() const;

	VkImageView GetTextureImageView(TextureHandle handle);
	VkSampler GetTextureSampler(TextureHandle handle);
	VkImageView GetFrameBufferColorImageView(FrameBufferHandle handle);
	VkSampler GetFrameBufferSampler(FrameBufferHandle handle);

  private:
	VulkanInstance m_instance;
	VulkanDevice m_device;
	ResourceManagerVulkan m_resourceManager;
	VkSurfaceKHR m_surface = VK_NULL_HANDLE;
	std::unique_ptr<VulkanRenderPass> m_presentRenderPass = nullptr;
	std::unique_ptr<VulkanSwapchain> m_swapchain = nullptr;
	std::vector<VkRenderPass> m_renderPasses = {};
	FrameBufferHandle m_activeFrameBuffer = {};
	VulkanRenderPass* m_currentRenderPass = nullptr;
	glm::uvec2 m_surfaceResolution = { 0, 0 };

	bool m_swapchainNeedsRecreate = false;

	VkCommandPool m_commandPool = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> m_commandBuffers = {};

	std::vector<VkSemaphore> m_imageAvailableSemaphores = {};
	std::vector<VkSemaphore> m_renderFinishedSemaphores = {};
	std::vector<VkFence> m_inFlightFences = {};
	uint32_t m_currentFrame = 0;
	uint32_t m_nextImageIndex = 0;

	struct RenderPassKey {
		VkFormat colorFormat;
		VkImageLayout finalColorLayout;

		bool operator==(const RenderPassKey& other) const {
			return colorFormat == other.colorFormat && finalColorLayout == other.finalColorLayout;
		}
	};

	struct RenderPassKeyHash {
		std::size_t operator()(const RenderPassKey& key) const {
			return std::hash<uint32_t>{}(static_cast<uint32_t>(key.colorFormat)) ^
			       (std::hash<uint32_t>{}(static_cast<uint32_t>(key.finalColorLayout)) << 1);
		}
	};
	std::unordered_map<RenderPassKey, std::unique_ptr<VulkanRenderPass>, RenderPassKeyHash>
	    m_renderPassCache;

	void InitVulkan();
	void Cleanup();

	void RecreateSwapChain();
	void CreateSyncObjects();

	VulkanRenderPass* GetOrCreateRenderPass(VkFormat colorFormat, VkImageLayout finalLayout);
};

} // namespace PixieRenderer
