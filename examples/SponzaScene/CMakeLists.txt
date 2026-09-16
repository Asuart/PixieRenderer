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

target_link_libraries(SponzaScene PRIVATE
    tf gf sdf vt
    usd usdGeom usdShade usdLux
)
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

# ---- Ресурсы проекта ----
add_custom_command(TARGET SponzaScene POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/resources/models/cube"
        "$<TARGET_FILE_DIR:SponzaScene>/cube"
)

# ============================================================================
# Runtime-развёртывание (Windows)
#
# Порядок команд критичен:
#   1) Python DLL — не CMake-таргет, копируем вручную
#   2) Плагины USD — это данные (plugInfo.json), копируем структуру
#   3) Широкое копирование ${USD_ROOT}/lib — на случай DLL, которые не
#      экспортированы как CMake-таргеты (MaterialX*, tbbmalloc* и т.п.)
#   4) TARGET_RUNTIME_DLLS — авторитетные версии из find_package(),
#      ПЕРЕЗАПИСЫВАЮТ всё, что положил шаг 3 (в т.ч. tbb12.dll).
#      ИДЁТ ПОСЛЕДНИМ.
# ============================================================================
if(WIN32)
    # 1. Python DLL
    if(NOT PYTHON_DLL_DIR OR NOT EXISTS "${PYTHON_DLL_DIR}/python310.dll")
        message(FATAL_ERROR
            "PYTHON_DLL_DIR must point to the folder containing python310.dll.\n"
            "Current value: '${PYTHON_DLL_DIR}'\n"
            "Set it via -DPYTHON_DLL_DIR=... or environment variable PYTHON_DLL_DIR.")
    endif()
    add_custom_command(TARGET SponzaScene POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${PYTHON_DLL_DIR}/python310.dll"
            "${PYTHON_DLL_DIR}/python3.dll"
            "$<TARGET_FILE_DIR:SponzaScene>"
        COMMENT "Copying Python DLLs next to executable"
    )

    # 2. Плагины USD
    add_custom_command(TARGET SponzaScene POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${USD_ROOT}/lib/usd"    "$<TARGET_FILE_DIR:SponzaScene>/lib/usd"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${USD_ROOT}/plugin/usd" "$<TARGET_FILE_DIR:SponzaScene>/plugin/usd"
        COMMENT "Copying USD plugins (plugInfo.json)"
    )

    # 3. Широкое копирование всех DLL из install-каталога USD
    add_custom_command(TARGET SponzaScene POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${USD_ROOT}/lib" "$<TARGET_FILE_DIR:SponzaScene>"
        COMMENT "Copying USD install lib/*.dll (broad sweep)"
    )

    # 4. Runtime DLL из CMake-таргетов — авторитетные версии.
    #    ВАЖНО: этот блок должен идти последним.
    add_custom_command(TARGET SponzaScene POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:SponzaScene>
            "$<TARGET_FILE_DIR:SponzaScene>"
        COMMAND_EXPAND_LISTS
        COMMENT "Copying runtime DLLs from CMake targets (authoritative)"
    )
endif()