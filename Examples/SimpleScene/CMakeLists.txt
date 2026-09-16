
file(GLOB_RECURSE _source_list CONFIGURE_DEPENDS "*.c" "*.cpp" "*.h" "*.hpp")

add_executable(SimpleScene ${_source_list})

set_target_properties(SimpleScene PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_include_directories(SimpleScene PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(SimpleScene PRIVATE PixieRenderer)

if (MSVC)
   target_compile_options(SimpleScene PRIVATE /W4)
else()
   target_compile_options(SimpleScene PRIVATE -Wall)
endif()

foreach(_source IN ITEMS ${_source_list})
    get_filename_component(_source_path "${_source}" PATH)
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" _group_path "${_source_path}")
    string(REPLACE "../" "" _group_path "${_group_path}")
    source_group("${_group_path}" FILES "${_source}")
endforeach()
