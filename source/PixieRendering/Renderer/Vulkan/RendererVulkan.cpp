#include "PixieRendering/pch.h"
#include "RendererVulkan.h"

#include "PixieRendering/Window/WindowVulkan.h"
#include "DebugVulkan.h"
#include "VulkanConfig.h"

namespace PixieRenderer {

RendererVulkan::RendererVulkan(IWindow* window)
    : IRenderer(window, RenderAPI::Vulkan), m_resourceManager(m_device) {
	InitVulkan();
	m_surfaceResolution = window->GetResolution();
}

void RendererVulkan::InitVulkan() {
	WindowVulkan* windowVulkan = reinterpret_cast<WindowVulkan*>(m_window);

	m_instance.Initialize(windowVulkan->GetRequiredExtensions());

	windowVulkan->CreateSurface(m_instance.GetInstance(), m_surface);
	if (m_surface == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to create surface!");
	}

	VkPhysicalDevice physicalDevice = m_instance.PickPhysicalDevice(m_surface);
	if (physicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to find a suitable GPU!");
	}

	const std::vector<const char*>& deviceExtensions = m_instance.GetDeviceExtensions();
	m_device.Initialize(physicalDevice, m_surface, deviceExtensions);

	m_presentRenderPass = std::make_unique<
	    VulkanRenderPass>(m_device, m_device.GetSurfaceFormat(), VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	m_swapchain = std::make_unique<VulkanSwapchain>(
	    m_device,
	    VkExtent2D{ m_surfaceResolution.x, m_surfaceResolution.y },
	    m_presentRenderPass->GetRenderPass()
	);

	m_device.CreateCommandPool(m_commandPool);

	m_commandBuffers.resize(cMaxFramesInFlight);
	for (size_t i = 0; i < m_commandBuffers.size(); i++) {
		m_device.CreateCommandBuffer(m_commandPool, m_commandBuffers[i]);
	}

	CreateSyncObjects();
}

void RendererVulkan::Cleanup() {
	m_device.WaitIdle();

	m_swapchain.reset();

	m_renderPasses.clear();

	for (size_t i = 0; i < cMaxFramesInFlight; i++) {
		m_device.DestroyFence(m_inFlightFences[i]);
		m_device.DestroySemaphore(m_imageAvailableSemaphores[i]);
		m_device.DestroySemaphore(m_renderFinishedSemaphores[i]);
	}

	m_device.DestroyCommandPool(m_commandPool);

	vkDestroySurfaceKHR(m_instance.GetInstance(), m_surface, nullptr);
}

bool RendererVulkan::BeginFrame() {
	if (m_swapchainNeedsRecreate) {
		if (m_surfaceResolution.x == 0 || m_surfaceResolution.y == 0) {
			return false;
		}
		RecreateSwapChain();
		m_swapchainNeedsRecreate = false;
	}

	m_device.WaitFences(1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);
	m_device.ResetFences(1, &m_inFlightFences[m_currentFrame]);

	VkResult result = vkAcquireNextImageKHR(
	    m_device.GetDevice(),
	    m_swapchain->GetSwapChain(),
	    UINT64_MAX,
	    m_imageAvailableSemaphores[m_currentFrame],
	    VK_NULL_HANDLE,
	    &m_nextImageIndex
	);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		RecreateSwapChain();
		return false;
	} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("Failed to acquire swap chain image!");
	}

	vkResetCommandBuffer(m_commandBuffers[m_currentFrame], 0);
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	if (vkBeginCommandBuffer(m_commandBuffers[m_currentFrame], &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("Failed to begin command buffer!");
	}

	m_currentRenderPass = nullptr;

	return true;
}

void RendererVulkan::EndFrame() {
	if (vkEndCommandBuffer(m_commandBuffers[m_currentFrame]) != VK_SUCCESS) {
		throw std::runtime_error("Failed to record command buffer!");
	}

	VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[m_currentFrame] };
	VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[m_nextImageIndex] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_commandBuffers[m_currentFrame];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(m_device.m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]) !=
	    VK_SUCCESS) {
		throw std::runtime_error("Failed to submit draw command buffer!");
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { m_swapchain->GetSwapChain() };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &m_nextImageIndex;

	VkResult result = vkQueuePresentKHR(m_device.m_presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		m_swapchainNeedsRecreate = true;
	} else if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to present swap chain image!");
	}

	m_currentFrame = (m_currentFrame + 1) % cMaxFramesInFlight;
}

void RendererVulkan::BeginRenderPass(FrameBufferHandle handle) {
	if (m_currentRenderPass) {
		return;
	}

	m_activeFrameBuffer = handle;

	VkFramebuffer framebuffer;
	VkExtent2D extent;
	VulkanRenderPass* rp = nullptr;
	VkViewport viewport{};
	VkRect2D scissor{};

	if (m_activeFrameBuffer) {
		VulkanFrameBuffer& fb = m_resourceManager.GetFrameBufferEntry(m_activeFrameBuffer);

		m_device.TransitionImage(
		    m_commandBuffers[m_currentFrame],
		    fb.GetColorImage(),
		    VK_IMAGE_LAYOUT_UNDEFINED,
		    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		    0,
		    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		    VK_IMAGE_ASPECT_COLOR_BIT,
		    1,
		    1
		);

		framebuffer = fb.GetFrameBuffer();
		extent = fb.GetExtent();
		rp = fb.GetRenderPassObject();
		viewport = fb.GetViewport();
		scissor = fb.GetScissor();
	} else {
		m_swapchain->Transition(
		    m_commandBuffers[m_currentFrame],
		    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		    VK_ACCESS_MEMORY_READ_BIT,
		    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		    VK_IMAGE_ASPECT_COLOR_BIT,
		    m_nextImageIndex
		);

		framebuffer = m_swapchain->GetFrameBuffer(m_nextImageIndex);
		extent = m_swapchain->GetExtent();
		rp = m_presentRenderPass.get();
		viewport = { 0.0f, 0.0f, (float)extent.width, (float)extent.height, 0.0f, 1.0f };
		scissor = { { 0, 0 }, extent };
	}

	rp->Begin(
	    m_commandBuffers[m_currentFrame],
	    m_currentFrame,
	    framebuffer,
	    extent,
	    viewport,
	    scissor
	);

	m_currentRenderPass = rp;

	debug_isInRenderPass = true;
}

void RendererVulkan::EndRenderPass() {
	if (!m_currentRenderPass) {
		return;
	}

	m_currentRenderPass
	    ->Execute(m_resourceManager.GetMeshes(), m_resourceManager.GetGraphicsPrograms());
	m_currentRenderPass->End();

	debug_isInRenderPass = false;

	if (m_activeFrameBuffer) {
		VulkanFrameBuffer& fb = m_resourceManager.GetFrameBufferEntry(m_activeFrameBuffer);
		fb.Transition(
		    m_commandBuffers[m_currentFrame],
		    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		    VK_ACCESS_SHADER_READ_BIT,
		    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		    VK_IMAGE_ASPECT_COLOR_BIT
		);
	} else {
		m_swapchain->SetImageLayout(m_nextImageIndex, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
	}

	m_currentRenderPass = nullptr;

	m_activeFrameBuffer = FrameBufferHandle();
}

void RendererVulkan::SetRenderResolution(glm::uvec2 resolution) {
	if (m_surfaceResolution == resolution) {
		return;
	}
	m_surfaceResolution = resolution;
	m_swapchainNeedsRecreate = true;
}

void RendererVulkan::SetViewport(glm::ivec2 start, glm::uvec2 resolution) {
	if (m_activeFrameBuffer) {
		VulkanFrameBuffer& fb = m_resourceManager.GetFrameBufferEntry(m_activeFrameBuffer);
		VkViewport viewport;
		viewport.width = static_cast<float>(resolution.x);
		viewport.height = static_cast<float>(resolution.y);
		viewport.x = static_cast<float>(start.x);
		viewport.y = static_cast<float>(start.y);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		fb.SetViewport(viewport);
	} else {
		// TODO
	}
}

void RendererVulkan::SetScissor(glm::ivec2 start, glm::uvec2 resolution) {
	if (m_activeFrameBuffer) {
		VulkanFrameBuffer& fb = m_resourceManager.GetFrameBufferEntry(m_activeFrameBuffer);
		VkRect2D scissor;
		scissor.offset = { start.x, start.y };
		scissor.extent = { resolution.x, resolution.y };
		fb.SetScissor(scissor);
	} else {
		// TODO
	}
}

MeshHandle RendererVulkan::CreateMesh(const Mesh* mesh) {
	return m_resourceManager.CreateMesh(mesh);
}

void RendererVulkan::LoadMesh(MeshHandle handle, const Mesh* mesh) {
	VulkanMesh& meshEntry = m_resourceManager.GetMeshEntry(handle);
	meshEntry.Load(mesh);
}

void RendererVulkan::DrawMesh(
    MeshHandle meshHandle,
    MaterialHandle materialHandle,
    void* pushConstantsData,
    uint32_t pushConstantdsDataSize
) {
	RenderRequest req{};
	req.meshHandle = meshHandle;
	req.materialHandle = materialHandle;

	if (pushConstantsData && pushConstantdsDataSize > 0) {
		if (pushConstantdsDataSize > RenderRequest::kPushConstantCapacity) {
			throw std::runtime_error("Push constant size exceeds capacity");
		}
		std::memcpy(req.pushConstants.data(), pushConstantsData, pushConstantdsDataSize);
		req.pushConstantsSize = pushConstantdsDataSize;
	}

	if (m_currentRenderPass) {
		m_currentRenderPass->AddRenderRequest(req);
	} else {
		m_presentRenderPass->AddRenderRequest(req);
	}
}

FrameBufferHandle RendererVulkan::CreateFrameBuffer(
    glm::uvec2 resolution,
    TextureFormat format,
    bool isPresentTarget
) {
	VkFormat vkFormat = ToVkFormat(format);
	VkImageLayout finalLayout = isPresentTarget ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	                                            : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	VulkanRenderPass* rp = GetOrCreateRenderPass(vkFormat, finalLayout);
	return m_resourceManager
	    .CreateFrameBuffer(VkExtent2D{ resolution.x, resolution.y }, vkFormat, rp);
}

void RendererVulkan::ResizeFrameBuffer(FrameBufferHandle handle, glm::uvec2 resolution) {
	VulkanFrameBuffer& fb = m_resourceManager.GetFrameBufferEntry(handle);
	fb.Resize({ resolution.x, resolution.y });
}

glm::uvec2 RendererVulkan::GetFrameBufferResolution(FrameBufferHandle handle) {
	VulkanFrameBuffer& fb = m_resourceManager.GetFrameBufferEntry(handle);
	VkExtent2D extent = fb.GetExtent();
	return { extent.width, extent.height };
}

TextureHandle RendererVulkan::CreateTexture(const Image2D* image, uint32_t mipLevels) {
	return m_resourceManager.CreateTexture(image, mipLevels);
}

void RendererVulkan::LoadTexture(TextureHandle handle, const Image2D* image) {
	VulkanTexture& textureEntry = m_resourceManager.GetTextureEntry(handle);
	textureEntry.Load(image);
}

void RendererVulkan::SetTextureFiltering(
    TextureHandle handle,
    TextureFiltering minFilter,
    TextureFiltering magFilter
) {
	VulkanTexture& textureEntry = m_resourceManager.GetTextureEntry(handle);
	textureEntry
	    .SetFiltering(ToVkFilter(minFilter), ToVkFilter(magFilter), ToVkMipmapMode(minFilter));
}

void RendererVulkan::SetTextureWrap(
    TextureHandle handle,
    TextureWrap wrapU,
    TextureWrap wrapV,
    TextureWrap wrapW
) {
	VulkanTexture& textureEntry = m_resourceManager.GetTextureEntry(handle);
	textureEntry.SetWrap(
	    ToVkSamplerAddressMode(wrapU),
	    ToVkSamplerAddressMode(wrapV),
	    ToVkSamplerAddressMode(wrapW)
	);
}

glm::ivec2 RendererVulkan::GetTextureResolution(TextureHandle handle) {
	VulkanTexture& textureEntry = m_resourceManager.GetTextureEntry(handle);
	return glm::ivec2(textureEntry.GetWidth(), textureEntry.GetHeight());
}

void RendererVulkan::BindTexture(
    MaterialHandle materialHandle,
    const std::string& name,
    TextureHandle textureHandle,
    uint32_t index
) {
	VulkanGraphicsProgram& material = m_resourceManager.GetGraphicsProgramEntry(materialHandle);
	VulkanTexture& texture = m_resourceManager.GetTextureEntry(textureHandle);

	texture.Transition(
	    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	    VK_ACCESS_MEMORY_READ_BIT,
	    VK_ACCESS_SHADER_READ_BIT,
	    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
	    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
	    VK_IMAGE_ASPECT_COLOR_BIT
	);

	material.BindTexture(name, textureHandle, texture, m_currentFrame, index);
}

void RendererVulkan::BindTexture(
    ComputeProgramHandle computeMaterialHandle,
    const std::string& name,
    TextureHandle textureHandle,
    uint32_t index
) {
	VulkanComputeProgram& prog = m_resourceManager.GetComputeProgramEntry(computeMaterialHandle);
	VulkanTexture& texture = m_resourceManager.GetTextureEntry(textureHandle);

	texture.Transition(
	    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	    VK_ACCESS_MEMORY_READ_BIT,
	    VK_ACCESS_SHADER_READ_BIT,
	    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
	    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
	    VK_IMAGE_ASPECT_COLOR_BIT
	);

	prog.BindTexture(name, textureHandle, texture, m_currentFrame, index);
}

ShaderStorageBufferHandle RendererVulkan::CreateShaderStorageBuffer(
    const uint8_t* data,
    uint32_t size
) {
	ShaderStorageBufferHandle handle = m_resourceManager.CreateShaderStorageBuffer(
	    size,
	    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	if (data != nullptr) {
		LoadShaderStorageBuffer(handle, data, size);
	}

	return handle;
}

void RendererVulkan::LoadShaderStorageBuffer(
    ShaderStorageBufferHandle handle,
    const uint8_t* data,
    uint32_t size
) {
	VulkanBuffer& buf = m_resourceManager.GetShaderStorageBufferEntry(handle);
	buf.Load(data, size);
}

uint32_t RendererVulkan::GetShaderStorageBufferSize(ShaderStorageBufferHandle handle) {
	VulkanBuffer& buf = m_resourceManager.GetShaderStorageBufferEntry(handle);
	return static_cast<uint32_t>(buf.GetSize());
}

std::vector<uint8_t> RendererVulkan::GetShaderStorageBufferData(
    ShaderStorageBufferHandle handle,
    uint32_t offset,
    uint32_t size
) {
	VulkanBuffer& buf = m_resourceManager.GetShaderStorageBufferEntry(handle);
	if (offset + size > buf.GetSize()) {
		throw std::runtime_error("Requested data range out of bounds");
	}
	if (size == 0) {
		return {};
	}

	VulkanBuffer stagingBuffer(
	    m_device,
	    size,
	    VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	VkCommandBuffer cmd = m_device.BeginSingleTimeCommands();

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = offset;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;
	vkCmdCopyBuffer(cmd, buf.GetBuffer(), stagingBuffer.GetBuffer(), 1, &copyRegion);

	m_device.EndSingleTimeCommands(cmd);

	std::vector<uint8_t> result(size);
	stagingBuffer.ReadData(result.data(), size);

	return result;
}

UniformBufferHandle RendererVulkan::CreateUniformBuffer(const uint8_t* data, uint32_t size) {
	UniformBufferHandle handle = m_resourceManager.CreateUniformBuffer(
	    size,
	    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	if (data != nullptr) {
		LoadUniformBuffer(handle, data, size);
	}

	return handle;
}

void RendererVulkan::LoadUniformBuffer(
    UniformBufferHandle handle,
    const uint8_t* data,
    uint32_t size
) {
	VulkanBuffer bufferEntry = m_resourceManager.GetUniformBufferEntry(handle);
	bufferEntry.Load(data, size);
}

void RendererVulkan::LoadUniformBuffer(
    MaterialHandle handle,
    const std::string& name,
    const void* data,
    size_t size
) {
	VulkanGraphicsProgram& material = m_resourceManager.GetGraphicsProgramEntry(handle);
	VulkanBuffer* buffer = material.GetUniformBuffer(name, m_currentFrame);
	if (buffer) {
		buffer->LoadSubData(data, size, 0);
	}
}

MaterialHandle RendererVulkan::CreateMaterial(const IMaterial* materialInfo) {
	return m_resourceManager
	    .CreateGraphicsProgram(m_presentRenderPass->GetRenderPass(), materialInfo);
}

ComputeProgramHandle RendererVulkan::CreateComputeProgram(const char* source) {
	return m_resourceManager.CreateComputeProgram(source);
}

void RendererVulkan::DispatchComputeProgram(
    ComputeProgramHandle handle,
    int32_t x,
    int32_t y,
    int32_t z
) {
	VulkanComputeProgram& prog = m_resourceManager.GetComputeProgramEntry(handle);
	VkCommandBuffer cmdBuf = m_commandBuffers[m_currentFrame];
	prog.Dispatch(cmdBuf, m_currentFrame, x, y, z);
}

void RendererVulkan::WaitIdle() {
	m_device.WaitIdle();
}

void RendererVulkan::MemoryBarriersAll() {
	VkMemoryBarrier memoryBarrier{};
	memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	memoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
	memoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;

	vkCmdPipelineBarrier(
	    m_commandBuffers[m_currentFrame],
	    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
	    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
	    0,
	    1,
	    &memoryBarrier,
	    0,
	    nullptr,
	    0,
	    nullptr
	);
}

VkInstance RendererVulkan::GetInstance() const {
	return m_instance.GetInstance();
}

VkPhysicalDevice RendererVulkan::GetPhysicalDevice() const {
	return m_device.GetPhysicalDevice();
}

VkDevice RendererVulkan::GetDevice() const {
	return m_device.GetDevice();
}

VkQueue RendererVulkan::GetGraphicsQueue() const {
	return m_device.GetGraphicsQueue();
}

VkQueue RendererVulkan::GetPresentQueue() const {
	return m_device.GetPresentQueue();
}

VkRenderPass RendererVulkan::GetPresentRenderPass() const {
	return m_presentRenderPass->GetRenderPass();
}

VkCommandBuffer RendererVulkan::GetCurrentFrameCommandBuffer() const {
	return m_commandBuffers[m_currentFrame];
}

VkImageView RendererVulkan::GetTextureImageView(TextureHandle handle) {
	VulkanTexture& texture = m_resourceManager.GetTextureEntry(handle);
	return texture.GetImageView();
}

VkSampler RendererVulkan::GetTextureSampler(TextureHandle handle) {
	VulkanTexture& texture = m_resourceManager.GetTextureEntry(handle);
	return texture.GetSampler();
}

VkImageView RendererVulkan::GetFrameBufferColorImageView(FrameBufferHandle handle) {
	VulkanFrameBuffer& fb = m_resourceManager.GetFrameBufferEntry(handle);
	return fb.GetColorImageView();
}

VkSampler RendererVulkan::GetFrameBufferSampler(FrameBufferHandle handle) {
	VulkanFrameBuffer& fb = m_resourceManager.GetFrameBufferEntry(handle);
	return fb.GetSampler();
}

void RendererVulkan::CreateSyncObjects() {
	uint32_t imageCount = static_cast<uint32_t>(m_swapchain->GetImageCount());

	m_imageAvailableSemaphores.resize(cMaxFramesInFlight);
	for (uint32_t i = 0; i < cMaxFramesInFlight; ++i) {
		m_device.CreateSemaphore(m_imageAvailableSemaphores[i]);
	}

	m_renderFinishedSemaphores.resize(imageCount);
	for (uint32_t i = 0; i < imageCount; ++i) {
		m_device.CreateSemaphore(m_renderFinishedSemaphores[i]);
	}

	m_inFlightFences.resize(cMaxFramesInFlight);
	for (uint32_t i = 0; i < cMaxFramesInFlight; ++i) {
		m_device.CreateFence(m_inFlightFences[i]);
	}
}

void RendererVulkan::RecreateSwapChain() {
	vkQueueWaitIdle(m_device.m_presentQueue);
	vkQueueWaitIdle(m_device.m_graphicsQueue);
	m_device.WaitIdle();

	for (VkSemaphore sem : m_renderFinishedSemaphores) {
		m_device.DestroySemaphore(sem);
	}
	m_renderFinishedSemaphores.clear();

	m_swapchain.reset();
	m_swapchain = std::make_unique<VulkanSwapchain>(
	    m_device,
	    VkExtent2D{ m_surfaceResolution.x, m_surfaceResolution.y },
	    m_presentRenderPass->GetRenderPass()
	);

	uint32_t imageCount = static_cast<uint32_t>(m_swapchain->GetImageCount());
	m_renderFinishedSemaphores.resize(imageCount);
	for (uint32_t i = 0; i < imageCount; ++i) {
		m_device.CreateSemaphore(m_renderFinishedSemaphores[i]);
	}

	SetViewport({ 0, 0 }, { m_surfaceResolution.x, m_surfaceResolution.y });
}

VulkanRenderPass* RendererVulkan::GetOrCreateRenderPass(
    VkFormat colorFormat,
    VkImageLayout finalLayout
) {
	RenderPassKey key{ colorFormat, finalLayout };
	auto it = m_renderPassCache.find(key);
	if (it != m_renderPassCache.end())
		return it->second.get();

	auto rp = std::make_unique<VulkanRenderPass>(m_device, colorFormat, finalLayout);
	VulkanRenderPass* ptr = rp.get();
	m_renderPassCache.emplace(key, std::move(rp));
	return ptr;
}

} // namespace PixieRenderer
