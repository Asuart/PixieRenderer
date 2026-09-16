#pragma once
#include <vulkan/vulkan.hpp>

#include "PixieRenderer/Image/ImageTypes.h"

namespace PixieRenderer {

class VulkanDevice;

class VulkanSampler {
  public:
	VulkanSampler(VulkanDevice& parentDevice);
	~VulkanSampler();

	VkSampler GetSampler() const;

	void
	SetWrap(VkSamplerAddressMode wrapU, VkSamplerAddressMode wrapV, VkSamplerAddressMode wrapW);
	void SetFiltering(VkFilter minFilter, VkFilter magFilter, VkSamplerMipmapMode mipmapMode);
	void SetAnisotropy(bool state);

  private:
	VulkanDevice& m_device;
	VkSampler m_sampler = VK_NULL_HANDLE;
	VkFilter m_minFilter = VK_FILTER_LINEAR;
	VkFilter m_magFilter = VK_FILTER_LINEAR;
	VkSamplerMipmapMode m_mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	VkSamplerAddressMode m_addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	VkSamplerAddressMode m_addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	VkSamplerAddressMode m_addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	bool m_anisotropyEnabled = true;

	void CreateVkSampler();
	void DestroyVkSampler();
};

static inline VkSamplerAddressMode ToVkSamplerAddressMode(TextureWrap wrap) {
	switch (wrap) {
	case TextureWrap::Repeat:
		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	case TextureWrap::MirroredRepeat:
		return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	case TextureWrap::ClampToEdge:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	case TextureWrap::ClampToBorder:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	default:
		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	}
}

static inline VkFilter ToVkFilter(TextureFiltering filtering) {
	switch (filtering) {
	case TextureFiltering::Linear:
		return VK_FILTER_LINEAR;
	case TextureFiltering::Nearest:
		return VK_FILTER_NEAREST;
	case TextureFiltering::NearestMipmapNearest:
		return VK_FILTER_NEAREST;
	case TextureFiltering::NearestMipmapLinear:
		return VK_FILTER_NEAREST;
	case TextureFiltering::LinearMipmapNearest:
		return VK_FILTER_LINEAR;
	case TextureFiltering::LinearMipmapLinear:
		return VK_FILTER_LINEAR;
	default:
		return VK_FILTER_LINEAR;
	}
}

static inline VkSamplerMipmapMode ToVkMipmapMode(TextureFiltering filtering) {
	switch (filtering) {
	case TextureFiltering::Linear:
		return VK_SAMPLER_MIPMAP_MODE_LINEAR;
	case TextureFiltering::Nearest:
		return VK_SAMPLER_MIPMAP_MODE_LINEAR;
	case TextureFiltering::NearestMipmapNearest:
		return VK_SAMPLER_MIPMAP_MODE_NEAREST;
	case TextureFiltering::NearestMipmapLinear:
		return VK_SAMPLER_MIPMAP_MODE_LINEAR;
	case TextureFiltering::LinearMipmapNearest:
		return VK_SAMPLER_MIPMAP_MODE_NEAREST;
	case TextureFiltering::LinearMipmapLinear:
		return VK_SAMPLER_MIPMAP_MODE_LINEAR;
	default:
		return VK_SAMPLER_MIPMAP_MODE_LINEAR;
	}
}

} // namespace PixieRenderer