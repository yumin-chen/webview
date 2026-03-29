cmake_minimum_required(VERSION 3.16)
project(alloy LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ALLOY_SOURCES
    main.cpp
    runtime.cpp
    cron_parser.cpp
)

if(APPLE)
    list(APPEND ALLOY_SOURCES cron_manager_macos.mm)
elseif(WIN32)
    list(APPEND ALLOY_SOURCES cron_manager_windows.cpp)
else()
    list(APPEND ALLOY_SOURCES cron_manager_linux.cpp)
endif()

add_executable(alloy_bin ${ALLOY_SOURCES})
target_include_directories(alloy_bin PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../core/include)
target_link_libraries(alloy_bin PRIVATE webview::core)

# Add webview include path
target_include_directories(alloy_bin PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../core/include)

if(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0 webkit2gtk-4.1)
    target_include_directories(alloy_bin PRIVATE ${GTK3_INCLUDE_DIRS})
    target_link_libraries(alloy_bin PRIVATE ${GTK3_LIBRARIES} dl)
elseif(APPLE)
    target_link_libraries(alloy_bin PRIVATE "-framework WebKit" dl)
elseif(WIN32)
    target_link_libraries(alloy_bin PRIVATE advapi32 ole32 shell32 shlwapi user32 version)
endif()
