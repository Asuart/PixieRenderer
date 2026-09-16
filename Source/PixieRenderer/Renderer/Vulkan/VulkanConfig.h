#pragma once
#include <cstdint>
#include <string>

#include <vulkan/vulkan.h>

namespace PixieRenderer {

static constexpr uint32_t cMaxFramesInFlight = 3;
static const std::string cApplicationName = "PixieEngine Application";
static const uint32_t cApplicationVersion = VK_MAKE_VERSION(1, 0, 0);
static const std::string cEngineName = "PixieEngine";
static const uint32_t cEngineVersion = VK_MAKE_VERSION(1, 0, 0);

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

} // namespace PixieRenderer
