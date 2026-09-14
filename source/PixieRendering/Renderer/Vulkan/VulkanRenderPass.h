#pragma once
#include <vector>

#include <vulkan/vulkan.h>

#include "PixieRendering/ResourceManager/ResourceHandles.h"
#include "PixieRendering/ResourceManager/IResourceManager.h"

#include "VulkanFrameBuffer.h"
#include "VulkanGraphicsProgram.h"
#include "VulkanMesh.h"

namespace PixieRenderer {

struct RenderRequest {
	MeshHandle meshHandle;
	MaterialHandle materialHandle;

	static constexpr uint32_t kPushConstantCapacity = 128;
	std::array<uint8_t, kPushConstantCapacity> pushConstants{};
	uint32_t pushConstantsSize = 0;
};

class VulkanDevice;

class VulkanRenderPass {
  public:
	VulkanRenderPass(VulkanDevice& parentDevice, VkFormat colorFormat, VkImageLayout finalLayout);
	~VulkanRenderPass();

	VkRenderPass GetRenderPass() const;

	void AddRenderRequest(RenderRequest request);

	void Begin(VkCommandBuffer cmdBuf, uint32_t currentFrame, VulkanFrameBuffer& frameBuffer);
	void Begin(
	    VkCommandBuffer cmdBuf,
	    uint32_t currentFrame,
	    VkFramebuffer frameBuffer,
	    VkExtent2D extent
	);
	void Begin(
	    VkCommandBuffer cmdBuf,
	    uint32_t currentFrame,
	    VkFramebuffer frameBuffer,
	    VkExtent2D extent,
		VkViewport viewport,
		VkRect2D scissor
	);
	void Execute(
	    std::vector<ResourceEntry<VulkanMesh>>& meshes,
	    std::vector<ResourceEntry<VulkanGraphicsProgram>>& materials
	);
	void End();

  private:
	VulkanDevice& m_device;
	VkRenderPass m_renderPass = VK_NULL_HANDLE;
	VkFormat m_colorFormat = VK_FORMAT_UNDEFINED;
	VkFormat m_depthFormat = VK_FORMAT_UNDEFINED;
	VkImageLayout m_finalLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	std::vector<RenderRequest> m_renderRequests = {};
	// Pass variable
	VkCommandBuffer m_currentCommandBuffer = VK_NULL_HANDLE;
	uint32_t m_currentFrame = 0;
};

} // namespace PixieRenderer
