#pragma once
#include "../IRenderer.h"

#include <memory>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "PixieRenderer/Material/IMaterial.h"
#include "PixieRenderer/RenderGraph/RenderGraphTypes.h"
#include "PixieRenderer/ResourceManager/ResourceManagerVulkan.h"
#include "VulkanDevice.h"
#include "VulkanInstance.h"
#include "VulkanSwapchain.h"

namespace PixieRenderer {

class IWindow;
class VulkanRenderPass;

class RendererVulkan : public IRenderer {
  public:
	RendererVulkan(IWindow* window);
	~RendererVulkan() override;

	void SetPresentOverlayHook(std::function<void()> fn) override {
		m_overlayHook = std::move(fn);
	}

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
	void UpdateBuffer(BufferHandle handle, std::span<const std::byte> data, size_t offset = 0) override;
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

	// Used by stages
	void BeginStage(std::string_view name, StageType type) override;
	void EndStage() override;

	void UseResource(TextureHandle h, ResourceUsage usage) override;
	void UseResource(BufferHandle h, ResourceUsage usage) override;
	void UseResource(FrameBufferHandle h, ResourceUsage usage) override;

	void SetStageRenderTarget(FrameBufferHandle fbo) override;
	void SetStageViewport(ScreenRect rect) override;
	void SetStageScissor(ScreenRect rect) override;

	void MemoryBarriersAll();

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

	struct ResourceState {
		VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkAccessFlags access = 0;
		VkPipelineStageFlags stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	};

  private:
	IWindow* m_window = nullptr;

	VulkanInstance m_instance;
	VulkanDevice m_device;
	ResourceManagerVulkan m_resourceManager;
	VkSurfaceKHR m_surface = VK_NULL_HANDLE;

	std::unique_ptr<VulkanRenderPass> m_presentRenderPass = nullptr;
	std::unique_ptr<VulkanSwapchain> m_swapchain = nullptr;

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

	VkViewport m_presentViewport = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };
	VkRect2D m_presentScissor = { { 0, 0 }, { 0, 0 } };
	bool m_presentViewportDirty = false;
	bool m_presentScissorDirty = false;

	std::string m_currentStageName;
	StageType m_currentStageType;

	std::unordered_map<uint64_t, ResourceState> m_resourceStates;

	std::function<void()> m_overlayHook;

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
	std::unordered_map<RenderPassKey, std::unique_ptr<VulkanRenderPass>, RenderPassKeyHash> m_renderPassCache;

	void InitVulkan();
	void Cleanup();

	void RecreateSwapChain();
	void CreateSyncObjects();

	VulkanRenderPass* GetOrCreateRenderPass(VkFormat colorFormat, VkImageLayout finalLayout);
};

} // namespace PixieRenderer
