#include "RendererVulkan.h"
#include "PixieRenderer/pch.h"

#include <cstring>
#include <stdexcept>

#include "DebugVulkan.h"
#include "PixieRenderer/Window/WindowVulkan.h"
#include "VulkanConfig.h"

namespace PixieRenderer {

namespace {

static RendererVulkan::ResourceState ToVkState(ResourceUsage u) {
	switch (u) {
	case ResourceUsage::Sampled:
		return { VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			     VK_ACCESS_SHADER_READ_BIT,
			     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };
	case ResourceUsage::StorageRead:
		return { VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };
	case ResourceUsage::StorageWrite:
		return { VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };
	case ResourceUsage::ColorAttachment:
		return { VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			     VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	case ResourceUsage::DepthAttachment:
		return { VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			     VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			     VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT };
	case ResourceUsage::CopySrc:
		return { VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT };
	case ResourceUsage::CopyDst:
		return { VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT };
	case ResourceUsage::Present:
		return { VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 0, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };
	case ResourceUsage::UniformRead:
		return { VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			     VK_ACCESS_UNIFORM_READ_BIT,
			     VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };
	case ResourceUsage::VertexRead:
		return { VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			     VK_ACCESS_SHADER_READ_BIT,
			     VK_PIPELINE_STAGE_VERTEX_SHADER_BIT };
	case ResourceUsage::IndexRead:
		return { VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			     VK_ACCESS_INDEX_READ_BIT,
			     VK_PIPELINE_STAGE_VERTEX_INPUT_BIT };
	case ResourceUsage::IndirectRead:
		return { VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			     VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
			     VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT };
	default:
		return { VK_IMAGE_LAYOUT_GENERAL, 0, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT };
	}
}

VkBufferUsageFlags ToVkBufferUsage(BufferType type) {
	switch (type) {
	case BufferType::Uniform:
		return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	case BufferType::Storage:
		return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	case BufferType::Vertex:
		return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	case BufferType::Index:
		return VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	}
	return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
}

VkMemoryPropertyFlags ToVkMemoryProps(BufferType type) {
	switch (type) {
	case BufferType::Uniform:
		return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	default:
		return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	}
}

} // namespace

RendererVulkan::RendererVulkan(IWindow* window) : m_window(window), m_resourceManager(m_device) {
	InitVulkan();
	m_surfaceResolution = window->GetResolution();
}

RendererVulkan::~RendererVulkan() {
	Cleanup();
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

	m_presentViewport = { 0.0f,
		                  0.0f,
		                  static_cast<float>(m_swapchain->GetExtent().width),
		                  static_cast<float>(m_swapchain->GetExtent().height),
		                  0.0f,
		                  1.0f };
	m_presentScissor = { { 0, 0 }, m_swapchain->GetExtent() };
}

void RendererVulkan::Cleanup() {
	if (m_device.GetDevice() == VK_NULL_HANDLE) {
		return;
	}

	m_device.WaitIdle();

	m_swapchain.reset();

	for (size_t i = 0; i < m_inFlightFences.size(); i++) {
		m_device.DestroyFence(m_inFlightFences[i]);
	}
	for (auto sem : m_imageAvailableSemaphores) {
		m_device.DestroySemaphore(sem);
	}
	for (auto sem : m_renderFinishedSemaphores) {
		m_device.DestroySemaphore(sem);
	}

	if (m_commandPool != VK_NULL_HANDLE) {
		m_device.DestroyCommandPool(m_commandPool);
		m_commandPool = VK_NULL_HANDLE;
	}

	m_presentRenderPass.reset();

	if (m_surface != VK_NULL_HANDLE) {
		vkDestroySurfaceKHR(m_instance.GetInstance(), m_surface, nullptr);
		m_surface = VK_NULL_HANDLE;
	}

	m_device.Cleanup();
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
	m_activeFrameBuffer = FrameBufferHandle();

	return true;
}

void RendererVulkan::EndFrame() {
	if (m_currentRenderPass) {
		throw std::runtime_error("EndFrame called with an open render pass. Call EndRenderPass first.");
	}

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

	if (vkQueueSubmit(m_device.GetGraphicsQueue(), 1, &submitInfo, m_inFlightFences[m_currentFrame]) != VK_SUCCESS) {
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

	VkResult result = vkQueuePresentKHR(m_device.GetPresentQueue(), &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		m_swapchainNeedsRecreate = true;
	} else if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to present swap chain image!");
	}

	m_currentFrame = (m_currentFrame + 1) % cMaxFramesInFlight;
}

void RendererVulkan::BeginRenderPass(FrameBufferHandle handle) {
	if (m_currentRenderPass)
		EndRenderPass();

	m_activeFrameBuffer = handle;

	VkFramebuffer framebuffer = VK_NULL_HANDLE;
	VkExtent2D extent{};
	VulkanRenderPass* rp = nullptr;
	VkViewport viewport{};
	VkRect2D scissor{};

	if (m_activeFrameBuffer) {
		UseResource(m_activeFrameBuffer, ResourceUsage::ColorAttachment);

		VulkanFrameBuffer& fb = m_resourceManager.GetFrameBuffer(m_activeFrameBuffer);
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
		viewport = m_presentViewport;
		scissor = m_presentScissor;
	}

	rp->Begin(m_commandBuffers[m_currentFrame], m_currentFrame, framebuffer, extent, viewport, scissor);
	m_currentRenderPass = rp;
	debug_isInRenderPass = true;
}

void RendererVulkan::EndRenderPass() {
	if (!m_currentRenderPass)
		return;

	m_currentRenderPass->Execute(m_resourceManager.GetMeshes(), m_resourceManager.GetGraphicsPrograms());

	if (!m_activeFrameBuffer && m_overlayHook)
		m_overlayHook();

	m_currentRenderPass->End();
	debug_isInRenderPass = false;

	if (m_activeFrameBuffer) {
		m_resourceStates[m_activeFrameBuffer.GetId()] = ResourceState{ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			                                                           VK_ACCESS_SHADER_READ_BIT,
			                                                           VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
			                                                               VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };
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

void RendererVulkan::SetViewport(ScreenRect rect) {
	if (!m_currentRenderPass) {
		throw std::runtime_error("SetViewport called outside of a render pass.");
	}
	VkViewport vp{};
	vp.x = static_cast<float>(rect.origin.x);
	vp.y = static_cast<float>(rect.origin.y);
	vp.width = static_cast<float>(rect.size.x);
	vp.height = static_cast<float>(rect.size.y);
	vp.minDepth = 0.0f;
	vp.maxDepth = 1.0f;

	if (m_activeFrameBuffer) {
		VulkanFrameBuffer& fb = m_resourceManager.GetFrameBuffer(m_activeFrameBuffer);
		fb.SetViewport(vp);
	} else {
		m_presentViewport = vp;
		m_presentViewportDirty = true;
	}
}

void RendererVulkan::SetScissor(ScreenRect rect) {
	if (!m_currentRenderPass) {
		throw std::runtime_error("SetScissor called outside of a render pass.");
	}

	VkRect2D sc{};
	sc.offset = { rect.origin.x, rect.origin.y };
	sc.extent = { rect.size.x, rect.size.y };

	if (m_activeFrameBuffer) {
		VulkanFrameBuffer& fb = m_resourceManager.GetFrameBuffer(m_activeFrameBuffer);
		fb.SetScissor(sc);
	} else {
		m_presentScissor = sc;
		m_presentScissorDirty = true;
	}
}

MeshHandle RendererVulkan::CreateMesh(const Mesh* mesh) {
	return m_resourceManager.CreateMesh(mesh);
}

void RendererVulkan::UpdateMesh(MeshHandle handle, const Mesh* mesh) {
	VulkanMesh& meshEntry = m_resourceManager.GetMesh(handle);
	meshEntry.Load(mesh);
}

void RendererVulkan::DrawMesh(DrawRequest request) {
	if (!m_currentRenderPass) {
		throw std::runtime_error("DrawMesh called outside of a render pass. Call BeginRenderPass first.");
	}

	RenderRequest req{};
	req.meshHandle = request.mesh;
	req.materialHandle = request.material;

	if (!request.inlineData.empty()) {
		if (request.inlineData.size() > cMaxRequestDataSize) {
			throw std::runtime_error("inlineData exceeds push constant capacity");
		}
		std::memcpy(req.pushConstants.data(), request.inlineData.data(), request.inlineData.size());
		req.pushConstantsSize = static_cast<uint32_t>(request.inlineData.size());
	}

	m_currentRenderPass->AddRenderRequest(req);
}

void RendererVulkan::DispatchComputeProgram(DispatchRequest request) {
	if (m_currentRenderPass) {
		throw std::runtime_error("DispatchComputeProgram called inside a render pass. Call EndRenderPass first.");
	}
	VulkanComputeProgram& prog = m_resourceManager.GetComputeProgram(request.program);
	prog.Dispatch(
	    m_commandBuffers[m_currentFrame],
	    m_currentFrame,
	    request.x,
	    request.y,
	    request.z,
	    request.inlineData.empty() ? nullptr : request.inlineData.data(),
	    static_cast<uint32_t>(request.inlineData.size())
	);
}

FrameBufferHandle RendererVulkan::CreateFrameBuffer(glm::uvec2 resolution, TextureFormat format) {
	VkFormat vkFormat = ToVkFormat(format);
	VkImageLayout finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	VulkanRenderPass* rp = GetOrCreateRenderPass(vkFormat, finalLayout);
	return m_resourceManager.CreateFrameBuffer(VkExtent2D{ resolution.x, resolution.y }, vkFormat, rp);
}

void RendererVulkan::SetFrameBufferResolution(FrameBufferHandle handle, glm::uvec2 resolution) {
	VulkanFrameBuffer& fb = m_resourceManager.GetFrameBuffer(handle);
	fb.Resize({ resolution.x, resolution.y });
	m_resourceStates.erase(handle.GetId());
}

glm::uvec2 RendererVulkan::GetFrameBufferResolution(FrameBufferHandle handle) {
	VulkanFrameBuffer& fb = m_resourceManager.GetFrameBuffer(handle);
	VkExtent2D extent = fb.GetExtent();
	return { extent.width, extent.height };
}

TextureHandle RendererVulkan::CreateTexture(const Image2D* image) {
	return m_resourceManager.CreateTexture(image, 1);
}

void RendererVulkan::UpdateTexture(TextureHandle handle, const Image2D* image) {
	VulkanTexture& textureEntry = m_resourceManager.GetTexture(handle);
	textureEntry.Load(image);
}

glm::uvec2 RendererVulkan::GetTextureResolution(TextureHandle handle) {
	VulkanTexture& textureEntry = m_resourceManager.GetTexture(handle);
	return glm::uvec2(textureEntry.GetWidth(), textureEntry.GetHeight());
}

void RendererVulkan::SetTextureFiltering(TextureHandle handle, TextureFiltering minFilter, TextureFiltering magFilter) {
	VulkanTexture& textureEntry = m_resourceManager.GetTexture(handle);
	textureEntry.SetFiltering(ToVkFilter(minFilter), ToVkFilter(magFilter), ToVkMipmapMode(minFilter));
}

void RendererVulkan::SetTextureWrap(TextureHandle handle, TextureWrap wrapU, TextureWrap wrapV, TextureWrap wrapW) {
	VulkanTexture& textureEntry = m_resourceManager.GetTexture(handle);
	textureEntry.SetWrap(ToVkSamplerAddressMode(wrapU), ToVkSamplerAddressMode(wrapV), ToVkSamplerAddressMode(wrapW));
}

BufferHandle RendererVulkan::CreateBuffer(BufferType type, size_t size) {
	BufferHandle handle = m_resourceManager.CreateBuffer(size, ToVkBufferUsage(type), ToVkMemoryProps(type));
	return handle;
}

BufferHandle RendererVulkan::CreateBuffer(BufferType type, std::span<const std::byte> data) {
	BufferHandle handle = m_resourceManager.CreateBuffer(data.size(), ToVkBufferUsage(type), ToVkMemoryProps(type));

	if (!data.empty()) {
		VulkanBuffer& buffer = m_resourceManager.GetBuffer(handle);
		buffer.Load(data.data(), data.size());
	}

	return handle;
}

void RendererVulkan::UpdateBuffer(BufferHandle handle, std::span<const std::byte> data, size_t offset) {
	if (data.empty()) {
		return;
	}
	VulkanBuffer& buffer = m_resourceManager.GetBuffer(handle);
	buffer.LoadSubData(data.data(), data.size(), offset);
}

size_t RendererVulkan::GetBufferSize(BufferHandle handle) {
	VulkanBuffer& buffer = m_resourceManager.GetBuffer(handle);
	return static_cast<size_t>(buffer.GetSize());
}

std::vector<std::byte> RendererVulkan::ReadBuffer(BufferHandle handle, MemoryExtent extent) {
	VulkanBuffer& buffer = m_resourceManager.GetBuffer(handle);

	if (extent.start + extent.size > buffer.GetSize()) {
		throw std::runtime_error("ReadBuffer: range out of bounds");
	}
	if (extent.size == 0) {
		return {};
	}

	std::vector<std::byte> result(extent.size);

	if (buffer.GetMappedData() != nullptr) {
		buffer.ReadData(result.data(), extent.size, extent.start);
		return result;
	}

	VulkanBuffer staging(
	    m_device,
	    extent.size,
	    VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	VkCommandBuffer cmd = m_device.BeginSingleTimeCommands();

	VkBufferCopy region{};
	region.srcOffset = extent.start;
	region.dstOffset = 0;
	region.size = extent.size;
	vkCmdCopyBuffer(cmd, buffer.GetBuffer(), staging.GetBuffer(), 1, &region);

	m_device.EndSingleTimeCommands(cmd);

	staging.ReadData(result.data(), extent.size, 0);
	return result;
}

MaterialHandle RendererVulkan::CreateMaterial(const IMaterial* materialInfo) {
	return m_resourceManager.CreateGraphicsProgram(m_presentRenderPass->GetRenderPass(), materialInfo);
}

ComputeProgramHandle RendererVulkan::CreateComputeProgram(const IComputeProgram* computeInfo) {
	if (!computeInfo || !computeInfo->source) {
		return {};
	}
	return m_resourceManager.CreateComputeProgram(computeInfo->source);
}

void RendererVulkan::BindTexture(
    MaterialHandle materialHandle,
    std::string_view name,
    TextureHandle textureHandle,
    uint32_t arrayIndex
) {
	if (!materialHandle || !textureHandle) {
		return;
	}
	VulkanGraphicsProgram& program = m_resourceManager.GetGraphicsProgram(materialHandle);
	VulkanTexture& texture = m_resourceManager.GetTexture(textureHandle);
	program.BindTexture(name, texture, m_currentFrame, arrayIndex);
}

void RendererVulkan::BindTexture(
    ComputeProgramHandle programHandle,
    std::string_view name,
    TextureHandle textureHandle,
    uint32_t arrayIndex
) {
	if (!programHandle || !textureHandle) {
		return;
	}
	VulkanComputeProgram& prog = m_resourceManager.GetComputeProgram(programHandle);
	VulkanTexture& texture = m_resourceManager.GetTexture(textureHandle);
	prog.BindTexture(name, texture, m_currentFrame, arrayIndex);
}

void RendererVulkan::BindTexture(
    MaterialHandle materialHandle,
    std::string_view name,
    FrameBufferHandle frameBufferHandle,
    uint32_t arrayIndex
) {
	VulkanGraphicsProgram& program = m_resourceManager.GetGraphicsProgram(materialHandle);
	VulkanFrameBuffer& fb = m_resourceManager.GetFrameBuffer(frameBufferHandle);
	program.BindTextureView(name, fb.GetColorImageView(), fb.GetSampler(), m_currentFrame, arrayIndex);
}

void RendererVulkan::BindTexture(
    ComputeProgramHandle programHandle,
    std::string_view name,
    FrameBufferHandle frameBufferHandle,
    uint32_t arrayIndex
) {
	VulkanComputeProgram& prog = m_resourceManager.GetComputeProgram(programHandle);
	VulkanFrameBuffer& fb = m_resourceManager.GetFrameBuffer(frameBufferHandle);
	prog.BindTextureView(name, fb.GetColorImageView(), fb.GetSampler(), m_currentFrame, arrayIndex);
}

void RendererVulkan::BindBuffer(
    MaterialHandle materialHandle,
    std::string_view name,
    BufferHandle bufferHandle,
    MemoryExtent range
) {
	VulkanGraphicsProgram& program = m_resourceManager.GetGraphicsProgram(materialHandle);
	VulkanBuffer& buffer = m_resourceManager.GetBuffer(bufferHandle);
	program.BindBuffer(name, buffer.GetBuffer(), range.start, range.size, m_currentFrame);
}

void RendererVulkan::BindBuffer(
    ComputeProgramHandle programHandle,
    std::string_view name,
    BufferHandle bufferHandle,
    MemoryExtent range
) {
	VulkanComputeProgram& program = m_resourceManager.GetComputeProgram(programHandle);
	VulkanBuffer& buffer = m_resourceManager.GetBuffer(bufferHandle);
	program.BindBuffer(name, buffer.GetBuffer(), range.start, range.size, m_currentFrame);
}

void RendererVulkan::WaitIdle() {
	m_device.WaitIdle();
}

size_t RendererVulkan::GetUniformBufferOffsetAlignment() const {
	VkPhysicalDeviceProperties props{};
	vkGetPhysicalDeviceProperties(m_device.GetPhysicalDevice(), &props);
	return static_cast<size_t>(props.limits.minUniformBufferOffsetAlignment);
}

size_t RendererVulkan::GetStorageBufferOffsetAlignment() const {
	VkPhysicalDeviceProperties props{};
	vkGetPhysicalDeviceProperties(m_device.GetPhysicalDevice(), &props);
	return static_cast<size_t>(props.limits.minStorageBufferOffsetAlignment);
}

void RendererVulkan::BeginStage(std::string_view name, StageType type) {
	m_currentStageName = name;
	m_currentStageType = type;
}

void RendererVulkan::UseResource(TextureHandle h, ResourceUsage u) {
	auto& state = m_resourceStates[h.GetId()];
	ResourceState want = ToVkState(u);

	if (state.layout == want.layout && state.access == want.access)
		return;

	VulkanTexture& tex = m_resourceManager.GetTexture(h);

	VkImageMemoryBarrier b{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	b.oldLayout = state.layout;
	b.newLayout = want.layout;
	b.srcAccessMask = state.access;
	b.dstAccessMask = want.access;
	b.image = tex.GetImage();
	b.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	vkCmdPipelineBarrier(m_commandBuffers[m_currentFrame], state.stage, want.stage, 0, 0, nullptr, 0, nullptr, 1, &b);

	state = want;
}

void RendererVulkan::UseResource(FrameBufferHandle h, ResourceUsage u) {
	if (!h) {
		return;
	}

	auto& state = m_resourceStates[h.GetId()];
	ResourceState want = ToVkState(u);
	if (state.layout == want.layout && state.access == want.access)
		return;

	VulkanFrameBuffer& fb = m_resourceManager.GetFrameBuffer(h);

	VkImageMemoryBarrier b{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	b.oldLayout = state.layout;
	b.newLayout = want.layout;
	b.srcAccessMask = state.access;
	b.dstAccessMask = want.access;
	b.image = fb.GetColorImage();
	b.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	vkCmdPipelineBarrier(m_commandBuffers[m_currentFrame], state.stage, want.stage, 0, 0, nullptr, 0, nullptr, 1, &b);
	state = want;
}

void RendererVulkan::UseResource(BufferHandle h, ResourceUsage u) {
	if (u != ResourceUsage::StorageRead && u != ResourceUsage::StorageWrite && u != ResourceUsage::UniformRead) {
		return;
	}

	VulkanBuffer& buf = m_resourceManager.GetBuffer(h);

	VkBufferMemoryBarrier b{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
	b.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
	b.dstAccessMask = (u == ResourceUsage::StorageWrite) ? VK_ACCESS_SHADER_WRITE_BIT : VK_ACCESS_SHADER_READ_BIT;
	b.buffer = buf.GetBuffer();
	b.offset = 0;
	b.size = VK_WHOLE_SIZE;

	vkCmdPipelineBarrier(
	    m_commandBuffers[m_currentFrame],
	    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
	    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
	    0,
	    0,
	    nullptr,
	    1,
	    &b,
	    0,
	    nullptr
	);
}

void RendererVulkan::SetStageRenderTarget(FrameBufferHandle fbo) {
	if (m_currentRenderPass) {
		EndRenderPass();
	}
	BeginRenderPass(fbo);
}

void RendererVulkan::EndStage() {
	if (m_currentRenderPass)
		EndRenderPass();
}

void RendererVulkan::SetStageViewport(ScreenRect rect) {
	if (!m_currentRenderPass)
		return;
	VkViewport vp{};
	vp.x = float(rect.origin.x);
	vp.y = float(rect.origin.y);
	vp.width = float(rect.size.x);
	vp.height = float(rect.size.y);
	vp.minDepth = 0.f;
	vp.maxDepth = 1.f;

	if (m_activeFrameBuffer) {
		m_resourceManager.GetFrameBuffer(m_activeFrameBuffer).SetViewport(vp);
	} else {
		m_presentViewport = vp;
	}
	vkCmdSetViewport(m_commandBuffers[m_currentFrame], 0, 1, &vp);
}

void RendererVulkan::SetStageScissor(ScreenRect rect) {
	if (!m_currentRenderPass) {
		return;
	}
	VkRect2D scissor{};
	scissor.offset.x = rect.origin.x;
	scissor.offset.y = rect.origin.y;
	scissor.extent.width = rect.size.x;
	scissor.extent.height = rect.size.y;

	if (m_activeFrameBuffer) {
		m_resourceManager.GetFrameBuffer(m_activeFrameBuffer).SetScissor(scissor);
	} else {
		m_presentScissor = scissor;
	}
	vkCmdSetScissor(m_commandBuffers[m_currentFrame], 0, 1, &scissor);
}

void RendererVulkan::MemoryBarriersAll() {
	VkMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;

	vkCmdPipelineBarrier(
	    m_commandBuffers[m_currentFrame],
	    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
	    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
	    0,
	    1,
	    &barrier,
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
	VulkanTexture& texture = m_resourceManager.GetTexture(handle);
	return texture.GetImageView();
}

VkSampler RendererVulkan::GetTextureSampler(TextureHandle handle) {
	VulkanTexture& texture = m_resourceManager.GetTexture(handle);
	return texture.GetSampler();
}

VkImageView RendererVulkan::GetFrameBufferColorImageView(FrameBufferHandle handle) {
	VulkanFrameBuffer& fb = m_resourceManager.GetFrameBuffer(handle);
	return fb.GetColorImageView();
}

VkSampler RendererVulkan::GetFrameBufferSampler(FrameBufferHandle handle) {
	VulkanFrameBuffer& fb = m_resourceManager.GetFrameBuffer(handle);
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
	vkQueueWaitIdle(m_device.GetPresentQueue());
	vkQueueWaitIdle(m_device.GetGraphicsQueue());
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

	VkExtent2D extent = m_swapchain->GetExtent();
	m_presentViewport = { 0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f };
	m_presentScissor = { { 0, 0 }, extent };
	m_presentViewportDirty = false;
	m_presentScissorDirty = false;
}

VulkanRenderPass* RendererVulkan::GetOrCreateRenderPass(VkFormat colorFormat, VkImageLayout finalLayout) {
	RenderPassKey key{ colorFormat, finalLayout };
	auto it = m_renderPassCache.find(key);
	if (it != m_renderPassCache.end()) {
		return it->second.get();
	}

	auto rp = std::make_unique<VulkanRenderPass>(m_device, colorFormat, finalLayout);
	VulkanRenderPass* ptr = rp.get();
	m_renderPassCache.emplace(key, std::move(rp));
	return ptr;
}

} // namespace PixieRenderer
