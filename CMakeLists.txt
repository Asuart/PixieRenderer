cmake_minimum_required(VERSION 3.22)

project(PixieRendering VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# ---- Настройки зависимостей ----
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(ENABLE_OPT OFF CACHE BOOL "Disable SPIR-V optimisation" FORCE)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
set(PXR_BUILD_MONOLITHIC ON CACHE BOOL "Build a monolithic library" FORCE)
set(PXR_ENABLE_PYTHON_SUPPORT OFF CACHE BOOL "Disable Python support" FORCE)
set(PXR_BUILD_TESTS OFF CACHE BOOL "Disable tests" FORCE)
set(PXR_BUILD_EXAMPLES OFF CACHE BOOL "Disable examples" FORCE)
set(PXR_BUILD_TUTORIALS OFF CACHE BOOL "Disable tutorials" FORCE)
set(PXR_BUILD_IMAGING OFF CACHE BOOL "Disable imaging (optional)" FORCE)

if (UNIX AND NOT APPLE)
    add_compile_definitions(VK_USE_PLATFORM_WAYLAND_KHR)
    set(GLFW_BUILD_X11     ON  CACHE BOOL "" FORCE)
    set(GLFW_BUILD_WAYLAND ON  CACHE BOOL "" FORCE)
endif()

find_package(Vulkan REQUIRED)

find_package(Python3 REQUIRED COMPONENTS Development)
include_directories(${Python3_INCLUDE_DIRS})

add_subdirectory(dependencies)
add_subdirectory(source)
add_subdirectory(examples)