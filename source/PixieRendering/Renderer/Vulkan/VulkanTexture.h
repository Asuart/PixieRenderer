#pragma once
#include <cstdint>
#include <memory>

#include <vulkan/vulkan.h>

#include "../../Resources/Image2D.h"
#include "PixieRendering/TextureEnums.h"
#include "VulkanConfig.h"
#include "VulkanSampler.h"

namespace PixieRenderer {

class VulkanDevice;

class VulkanTexture {
  public:
	VulkanTexture(
	    VulkanDevice& parentDevice,
	    const Image2D* image = nullptr,
	    uint32_t mipmapLevels = 1
	);
	~VulkanTexture();

	void Load(const Image2D* image, uint32_t mipmapLevels = 1);
	void Free();

	uint32_t GetWidth() const;
	uint32_t GetHeight() const;
	uint32_t GetMipLevels() const;
	VkFormat GetFormat() const;
	VkImageView GetImageView() const;
	VkSampler GetSampler() const;
	VkImage GetImage() const;

	void SetSampler(const std::shared_ptr<VulkanSampler>& sampler);
	void SetWrap(
	    VkSamplerAddressMode wrapU,
	    VkSamplerAddressMode wrapV,
	    VkSamplerAddressMode wrapW
	);
	void SetFiltering(VkFilter minFilter, VkFilter magFilter, VkSamplerMipmapMode mipmapMode);
	void SetAnisatropy(bool state);

	void Transition(
	    VkImageLayout newLayout,
	    VkAccessFlags srcAccessMask,
	    VkAccessFlags dstAccessMask,
	    VkPipelineStageFlags srcStage,
	    VkPipelineStageFlags dstStage,
	    VkImageAspectFlags aspectMask
	);

  private:
	VulkanDevice& m_device;
	uint32_t m_width = 0;
	uint32_t m_height = 0;
	uint32_t m_mipLevels = 0;
	VkImage m_image = VK_NULL_HANDLE;
	VkDeviceMemory m_memory = VK_NULL_HANDLE;
	VkImageView m_imageView = VK_NULL_HANDLE;
	VkFormat m_format = VK_FORMAT_UNDEFINED;
	std::shared_ptr<VulkanSampler> m_sampler = nullptr;
	VkImageLayout m_imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	void GenerateMipmaps();
};

static inline VkFormat ToVkFormat(TextureFormat format) {
	switch (format) {
	case TextureFormat::Red8:
		return VK_FORMAT_R8_SRGB;
	case TextureFormat::RGB8:
		return VK_FORMAT_R8G8B8A8_SRGB;
	case TextureFormat::RGBA8:
		return VK_FORMAT_R8G8B8A8_SRGB;
	case TextureFormat::Red32f:
		return VK_FORMAT_R32_SFLOAT;
	case TextureFormat::RGB32f:
		return VK_FORMAT_R32G32B32_SFLOAT;
	case TextureFormat::RGBA32f:
		return VK_FORMAT_R32G32B32A32_SFLOAT;
	default:
		return VK_FORMAT_R32G32B32A32_SFLOAT;
	}
}

} // namespace PixieRenderer
