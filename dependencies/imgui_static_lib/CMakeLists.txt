add_library(imgui STATIC
  ../imgui/imgui.cpp
  ../imgui/imgui.h
  ../imgui/imconfig.h
  ../imgui/imgui_demo.cpp
  ../imgui/imgui_draw.cpp
  ../imgui/imgui_internal.h
  ../imgui/imgui_tables.cpp
  ../imgui/imgui_widgets.cpp
  ../imgui/imstb_rectpack.h
  ../imgui/imstb_textedit.h
  ../imgui/imstb_truetype.h
  ../imgui/backends/imgui_impl_glfw.h
  ../imgui/backends/imgui_impl_glfw.cpp
  ../imgui/backends/imgui_impl_opengl3.h
  ../imgui/backends/imgui_impl_opengl3.cpp
  ../imgui/backends/imgui_impl_vulkan.h
  ../imgui/backends/imgui_impl_vulkan.cpp
)

set_target_properties(imgui PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

find_package(Vulkan)

target_include_directories(imgui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../imgui")

target_link_libraries(imgui PUBLIC glfw)
target_link_libraries(imgui PUBLIC Vulkan::Vulkan)

if (MSVC)
    # target_compile_options(imgui PRIVATE /W4 /WX)
else()
    target_compile_options(imgui PRIVATE -Wall -Werror)
endif()
