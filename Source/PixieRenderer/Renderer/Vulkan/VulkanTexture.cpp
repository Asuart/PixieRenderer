#include "VulkanTexture.h"
#include "PixieRenderer/pch.h"

#include "PixieRenderer/Image/ImageUtils.h"
#include "VulkanBuffer.h"
#include "VulkanDevice.h"
#include "VulkanPhysicalDeviceUtils.h"
#include "VulkanSampler.h"

namespace PixieRenderer {

VulkanTexture::VulkanTexture(VulkanDevice& parentDevice, const Image2D* image, uint32_t mipmapLevels)
    : m_device(parentDevice) {
	if (image != nullptr) {
		Load(image, mipmapLevels);
	}
}

VulkanTexture::~VulkanTexture() {
	Free();
}

void VulkanTexture::Load(const Image2D* image, uint32_t mipmapLevels) {
	if (!image) {
		throw std::invalid_argument("image is null");
	}

	Free();

	m_width = image->resolution.x;
	m_height = image->resolution.y;
	m_format = ToVkFormat(image->format);

	uint32_t maxMip = static_cast<uint32_t>(std::floor(std::log2(std::max(m_width, m_height)))) + 1;
	m_mipLevels = std::clamp(mipmapLevels, 1u, maxMip);

	m_device.CreateImage(
	    m_width,
	    m_height,
	    m_mipLevels,
	    VK_SAMPLE_COUNT_1_BIT,
	    m_format,
	    VK_IMAGE_TILING_OPTIMAL,
	    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
	        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
	    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	    m_image,
	    m_memory
	);

	const bool hasPixels = !image->pixels.empty();

	if (hasPixels) {
		VkDeviceSize imageSize = m_width * m_height * FormatToByteSize(image->format);
		VulkanBuffer stagingBuffer(
		    m_device,
		    imageSize,
		    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		);
		stagingBuffer.Load(image->pixels.data(), imageSize);

		m_device.TransitionImageSingleTime(
		    m_image,
		    VK_IMAGE_LAYOUT_UNDEFINED,
		    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		    0,
		    VK_ACCESS_TRANSFER_WRITE_BIT,
		    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		    VK_PIPELINE_STAGE_TRANSFER_BIT,
		    VK_IMAGE_ASPECT_COLOR_BIT,
		    1,
		    1
		);

		m_device.CopyBufferToImage(stagingBuffer.GetBuffer(), m_image, m_width, m_height);
	} else {
		m_imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	}

	m_device.CreateImageView(m_image, m_format, VK_IMAGE_ASPECT_COLOR_BIT, m_mipLevels, m_imageView);

	if (hasPixels && m_mipLevels > 1) {
		GenerateMipmaps();
	}

	if (hasPixels) {
		m_device.TransitionImageSingleTime(
		    m_image,
		    m_imageLayout,
		    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		    VK_ACCESS_TRANSFER_WRITE_BIT,
		    VK_ACCESS_SHADER_READ_BIT,
		    VK_PIPELINE_STAGE_TRANSFER_BIT,
		    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		    VK_IMAGE_ASPECT_COLOR_BIT,
		    1,
		    1
		);
		m_imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}

	if (!m_sampler) {
		m_sampler = std::make_shared<VulkanSampler>(m_device);
	}
}

void VulkanTexture::Free() {
	VkDevice device = m_device.GetDevice();

	m_width = 0;
	m_height = 0;
	m_mipLevels = 0;
	m_format = VK_FORMAT_UNDEFINED;

	if (m_imageView != VK_NULL_HANDLE) {
		vkDestroyImageView(device, m_imageView, nullptr);
		m_imageView = VK_NULL_HANDLE;
	}
	if (m_image != VK_NULL_HANDLE) {
		vkDestroyImage(device, m_image, nullptr);
		m_image = VK_NULL_HANDLE;
	}
	if (m_memory != VK_NULL_HANDLE) {
		vkFreeMemory(device, m_memory, nullptr);
		m_memory = VK_NULL_HANDLE;
	}
}

uint32_t VulkanTexture::GetWidth() const {
	return m_width;
}

uint32_t VulkanTexture::GetHeight() const {
	return m_height;
}

uint32_t VulkanTexture::GetMipLevels() const {
	return m_mipLevels;
}

VkFormat VulkanTexture::GetFormat() const {
	return m_format;
}

VkImageView VulkanTexture::GetImageView() const {
	return m_imageView;
}

VkSampler VulkanTexture::GetSampler() const {
	return m_sampler != nullptr ? m_sampler->GetSampler() : VK_NULL_HANDLE;
}

VkImage VulkanTexture::GetImage() const {
	return m_image;
}

void VulkanTexture::SetSampler(const std::shared_ptr<VulkanSampler>& sampler) {
	m_sampler = sampler;
}

void VulkanTexture::SetWrap(VkSamplerAddressMode wrapU, VkSamplerAddressMode wrapV, VkSamplerAddressMode wrapW) {
	if (m_sampler == nullptr) {
		return;
	}
	m_sampler->SetWrap(wrapU, wrapV, wrapW);
}

void VulkanTexture::SetFiltering(VkFilter minFilter, VkFilter magFilter, VkSamplerMipmapMode mipmapMode) {
	if (m_sampler == nullptr) {
		return;
	}
	m_sampler->SetFiltering(minFilter, magFilter, mipmapMode);
}

void VulkanTexture::SetAnisatropy(bool state) {
	if (m_sampler == nullptr) {
		return;
	}
	m_sampler->SetAnisotropy(state);
}

void VulkanTexture::Transition(
    VkImageLayout newLayout,
    VkAccessFlags srcAccessMask,
    VkAccessFlags dstAccessMask,
    VkPipelineStageFlags srcStage,
    VkPipelineStageFlags dstStage,
    VkImageAspectFlags aspectMask
) {
	m_device.TransitionImageSingleTime(
	    m_image,
	    m_imageLayout,
	    newLayout,
	    srcAccessMask,
	    dstAccessMask,
	    srcStage,
	    dstStage,
	    aspectMask,
	    1,
	    m_mipLevels
	);
	m_imageLayout = newLayout;
}

void VulkanTexture::GenerateMipmaps() {
	if (m_mipLevels <= 1) {
		return;
	}

	VkPhysicalDevice physicalDevice = m_device.GetPhysicalDevice();

	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(physicalDevice, m_format, &formatProperties);

	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
		throw std::runtime_error("texture image format does not support linear blitting!");
	}

	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT) ||
	    !(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT)) {
		throw std::runtime_error("Texture image format does not support blit operations!");
	}

	VkCommandBuffer commandBuffer = m_device.BeginSingleTimeCommands();

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = m_image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = m_width;
	int32_t mipHeight = m_height;

	for (uint32_t i = 1; i < m_mipLevels; i++) {
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(
		    commandBuffer,
		    VK_PIPELINE_STAGE_TRANSFER_BIT,
		    VK_PIPELINE_STAGE_TRANSFER_BIT,
		    0,
		    0,
		    nullptr,
		    0,
		    nullptr,
		    1,
		    &barrier
		);

		VkImageBlit blit{};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(
		    commandBuffer,
		    m_image,
		    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		    m_image,
		    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		    1,
		    &blit,
		    VK_FILTER_LINEAR
		);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(
		    commandBuffer,
		    VK_PIPELINE_STAGE_TRANSFER_BIT,
		    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		    0,
		    0,
		    nullptr,
		    0,
		    nullptr,
		    1,
		    &barrier
		);

		if (mipWidth > 1) {
			mipWidth /= 2;
		}
		if (mipHeight > 1) {
			mipHeight /= 2;
		}
	}

	barrier.subresourceRange.baseMipLevel = m_mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(
	    commandBuffer,
	    VK_PIPELINE_STAGE_TRANSFER_BIT,
	    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
	    0,
	    0,
	    nullptr,
	    0,
	    nullptr,
	    1,
	    &barrier
	);

	m_device.EndSingleTimeCommands(commandBuffer);
}

} // namespace PixieRenderer
