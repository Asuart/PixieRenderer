cmake_minimum_required(VERSION 3.22)

# vcpkg подключаем только если он реально есть на диске
if(DEFINED ENV{VCPKG_ROOT}
   AND EXISTS "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
   AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

project(PixieRendering VERSION 1.0.0 LANGUAGES C CXX)

# ---- Конфигурационные переменные (передаются через -D или env) ----
set(USD_ROOT       "$ENV{USD_ROOT}"       CACHE PATH "Path to OpenUSD installation (contains pxrConfig.cmake)")
set(PYTHON_DLL_DIR "$ENV{PYTHON_DLL_DIR}" CACHE PATH "Directory containing python310.dll and python3.dll")

if(NOT USD_ROOT OR NOT EXISTS "${USD_ROOT}/pxrConfig.cmake")
    message(FATAL_ERROR
        "USD_ROOT must point to OpenUSD installation directory that contains pxrConfig.cmake.\n"
        "Current value: '${USD_ROOT}'\n"
        "Set it via -DUSD_ROOT=... or environment variable USD_ROOT.")
endif()

list(APPEND CMAKE_PREFIX_PATH "${USD_ROOT}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# ---- Настройки зависимостей ----
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(ENABLE_OPT OFF CACHE BOOL "Disable SPIR-V optimisation" FORCE)

if (UNIX AND NOT APPLE)
    add_compile_definitions(VK_USE_PLATFORM_WAYLAND_KHR)
    set(GLFW_BUILD_X11     ON  CACHE BOOL "" FORCE)
    set(GLFW_BUILD_WAYLAND ON  CACHE BOOL "" FORCE)
endif()

add_subdirectory(dependencies)

find_package(pxr CONFIG REQUIRED)

# TBB в сборке USD существует только в Release-варианте.
# В Debug-конфигурации перенаправляем на Release, чтобы линкер
# не искал несуществующий tbb12_debug.lib.
if(TARGET TBB::tbb)
    set_target_properties(TBB::tbb PROPERTIES
        MAP_IMPORTED_CONFIG_DEBUG          Release
        MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
        MAP_IMPORTED_CONFIG_MINSIZEREL     Release
    )
    message(STATUS "TBB::tbb mapped to Release for all configurations")
endif()

find_package(Vulkan REQUIRED)

add_subdirectory(source)
add_subdirectory(examples)