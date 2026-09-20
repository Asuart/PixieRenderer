
file(GLOB_RECURSE _source_list CONFIGURE_DEPENDS "*.c" "*.cpp" "*.h" "*.hpp")

add_library(PixieRenderer STATIC ${_source_list})

set_target_properties(PixieRenderer PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_precompile_headers(PixieRenderer PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/pch.h"
)

target_include_directories(PixieRenderer PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../")
target_include_directories(PixieRenderer PUBLIC "${glm_SOURCE_DIR}")

target_link_libraries(PixieRenderer PUBLIC glfw)
target_link_libraries(PixieRenderer PUBLIC glad)
target_link_libraries(PixieRenderer PUBLIC Vulkan::Vulkan)

target_link_libraries(PixieRenderer PRIVATE meshoptimizer)
target_link_libraries(PixieRenderer PRIVATE glslang)
target_link_libraries(PixieRenderer PRIVATE glslang-default-resource-limits)
target_link_libraries(PixieRenderer PRIVATE spirv-reflect-static)

if (MSVC)
    target_compile_options(PixieRenderer PRIVATE /W4 /WX)
else()
    target_compile_options(PixieRenderer PRIVATE -Wall -Werror)
endif()

foreach(_source IN ITEMS ${_source_list})
    get_filename_component(_source_path "${_source}" PATH)
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" _group_path "${_source_path}")
    string(REPLACE "../" "" _group_path "${_group_path}")
    source_group("${_group_path}" FILES "${_source}")
endforeach()
