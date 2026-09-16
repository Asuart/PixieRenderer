
file(GLOB_RECURSE _source_list CONFIGURE_DEPENDS "*.c" "*.cpp" "*.h" "*.hpp")

add_executable(SponzaScene ${_source_list})

set_target_properties(SponzaScene PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_include_directories(SponzaScene PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(SponzaScene PRIVATE "${CMAKE_SOURCE_DIR}/dependencies/tinyobjloader/")
target_include_directories(SponzaScene PRIVATE "${CMAKE_SOURCE_DIR}/dependencies/stb/")

target_link_libraries(SponzaScene PRIVATE pxr::usd)
target_link_libraries(SponzaScene PRIVATE EnTT::EnTT)
target_link_libraries(SponzaScene PRIVATE PixieUIApplication)

if (MSVC)
   target_compile_options(SponzaScene PRIVATE /W4)
else()
   target_compile_options(SponzaScene PRIVATE -Wall)
endif()

foreach(_source IN ITEMS ${_source_list})
    get_filename_component(_source_path "${_source}" PATH)
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" _group_path "${_source_path}")
    string(REPLACE "../" "" _group_path "${_group_path}")
    source_group("${_group_path}" FILES "${_source}")
endforeach()

add_custom_command(TARGET SponzaScene POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/resources/models/cube"
    "$<TARGET_FILE_DIR:SponzaScene>/cube"
)
