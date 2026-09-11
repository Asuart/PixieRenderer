cmake_minimum_required(VERSION 3.22)

project(PixieRendering VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# settings for dependencies
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

find_package(Vulkan REQUIRED)

add_subdirectory(source)
add_subdirectory(examples)
