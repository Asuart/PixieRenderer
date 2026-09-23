#pragma once
#include <PixieLog/Log.h>

namespace PixieRenderer {

using namespace PixieLog;

namespace LogCat {

// Vulkan
inline const LogCategory RendererVulkan{ "RendererVulkan" };
inline const LogCategory vkInstance{ "vkInstance" };
inline const LogCategory vkPhysicalDeviceUtils{ "vkPhysicalDeviceUtils" };
inline const LogCategory vkBuffer{ "vkBuffer" };

inline const LogCategory RendererOpenGL{ "RendererOpenGL" };
inline const LogCategory glFrameBuffer{ "glFrameBuffer" };
inline const LogCategory glTexture{ "glTexture" };

} // namespace LogCat

} // namespace PixieRenderer
