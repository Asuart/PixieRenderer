
file(GLOB_RECURSE _source_list CONFIGURE_DEPENDS "*.c" "*.cpp" "*.h" "*.hpp")

add_library(PixieRendering STATIC ${_source_list})

set_target_properties(PixieRendering PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_precompile_headers(PixieRendering PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/pch.h"
)

target_include_directories(PixieRendering PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../")
target_include_directories(PixieRendering PUBLIC "${glm_SOURCE_DIR}")

target_link_libraries(PixieRendering PUBLIC glfw)

target_link_libraries(PixieRendering PRIVATE meshoptimizer)
target_link_libraries(PixieRendering PRIVATE glad)
target_link_libraries(PixieRendering PRIVATE glslang)
target_link_libraries(PixieRendering PRIVATE glslang-default-resource-limits)
target_link_libraries(PixieRendering PRIVATE Vulkan::Vulkan)

if(WIN32)
    target_link_libraries(PixieRendering PRIVATE 
        debug "$ENV{VULKAN_SDK}/Lib/spirv-cross-cored.lib"
        optimized "$ENV{VULKAN_SDK}/Lib/spirv-cross-core.lib"
        
        debug "$ENV{VULKAN_SDK}/Lib/spirv-cross-glsld.lib"
        optimized "$ENV{VULKAN_SDK}/Lib/spirv-cross-glsl.lib"
    )
else()
    target_link_libraries(PixieRendering PRIVATE spirv-cross-core spirv-cross-glsl)
endif()

if (MSVC)
   target_compile_options(PixieRendering PRIVATE /W4 /WX)
else()
   target_compile_options(PixieRendering PRIVATE -Wall -Werror)
endif()

foreach(_source IN ITEMS ${_source_list})
    get_filename_component(_source_path "${_source}" PATH)
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" _group_path "${_source_path}")
    string(REPLACE "../" "" _group_path "${_group_path}")
    source_group("${_group_path}" FILES "${_source}")
endforeach()
