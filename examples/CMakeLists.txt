find_package(Threads REQUIRED)

set(COPYRIGHT "Copyright [year], Your Name")
set(VENDOR "Your Name")
set(DESCRIPTION "Example app made with webview")
set(SHARED_SOURCES)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    include(resources/macos/resources.cmake)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    include(resources/windows/resources.cmake)
endif()

add_executable(webview_example_basic_c MACOSX_BUNDLE WIN32)
target_sources(webview_example_basic_c PRIVATE basic.c ${SHARED_SOURCES})
target_link_libraries(webview_example_basic_c PRIVATE webview::core_static)

add_executable(webview_example_basic_cc MACOSX_BUNDLE WIN32)
target_sources(webview_example_basic_cc PRIVATE basic.cc ${SHARED_SOURCES})
target_link_libraries(webview_example_basic_cc PRIVATE webview::core)

add_executable(webview_example_bind_c MACOSX_BUNDLE WIN32)
target_sources(webview_example_bind_c PRIVATE bind.c ${SHARED_SOURCES})
target_link_libraries(webview_example_bind_c PRIVATE webview::core_static Threads::Threads)

if(MSVC)
    # Disable some warnings when using MSVC
    target_compile_definitions(webview_example_bind_c PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

add_executable(webview_example_bind_cc MACOSX_BUNDLE WIN32)
target_sources(webview_example_bind_cc PRIVATE bind.cc ${SHARED_SOURCES})
target_link_libraries(webview_example_bind_cc PRIVATE webview::core Threads::Threads)

add_executable(webview_alloy_test MACOSX_BUNDLE WIN32)
target_sources(webview_alloy_test PRIVATE alloy_test.cc ${SHARED_SOURCES})
target_link_libraries(webview_alloy_test PRIVATE webview::core Threads::Threads)

add_executable(webview_alloy_shell_test MACOSX_BUNDLE WIN32)
target_sources(webview_alloy_shell_test PRIVATE alloy_shell_test.cc ${SHARED_SOURCES})
target_link_libraries(webview_alloy_shell_test PRIVATE webview::core Threads::Threads)

add_executable(alloy_gui_example_c MACOSX_BUNDLE WIN32)
target_sources(alloy_gui_example_c PRIVATE gui.c)
target_include_directories(alloy_gui_example_c PRIVATE ../core/include)
target_link_libraries(alloy_gui_example_c PRIVATE alloy_gui)

add_executable(alloy_gui_example_cc MACOSX_BUNDLE WIN32)
target_sources(alloy_gui_example_cc PRIVATE gui.cc)
target_include_directories(alloy_gui_example_cc PRIVATE ../core/include)
target_link_libraries(alloy_gui_example_cc PRIVATE alloy_gui)

# Add TypeScript example build if possible
if(NOT WIN32)
    # On non-Windows, we can try to find bun to build the TS example
    find_program(BUN_EXE bun)
    if(BUN_EXE)
        add_custom_command(
            OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/gui.js"
            COMMAND ${BUN_EXE} build "${CMAKE_CURRENT_SOURCE_DIR}/gui.ts" --outfile "${CMAKE_CURRENT_BINARY_DIR}/gui.js"
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/gui.ts"
            COMMENT "Building TypeScript GUI example..."
        )
        add_custom_target(alloy_gui_example_ts ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/gui.js")
    endif()
endif()
