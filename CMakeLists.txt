cmake_minimum_required(VERSION 3.16)
project(alloy_host)

set(CMAKE_CXX_STANDARD 17)

# Find OpenSSL
find_package(OpenSSL REQUIRED)
# Find GTK and WebKit
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(WEBKIT2 REQUIRED webkit2gtk-4.1)

# MicroQuickJS
add_library(mquickjs STATIC
    core/mquickjs/mquickjs.c
    core/mquickjs/mqjs_stdlib.c
    core/mquickjs/dtoa.c
)
target_include_directories(mquickjs PUBLIC core/mquickjs)

# Webview
add_library(webview_static STATIC core/webview/src/webview.cc)
target_include_directories(webview_static PUBLIC core/webview/include ${GTK3_INCLUDE_DIRS} ${WEBKIT2_INCLUDE_DIRS})
target_compile_definitions(webview_static PUBLIC WEBVIEW_STATIC)
target_link_libraries(webview_static PRIVATE ${GTK3_LIBRARIES} ${WEBKIT2_LIBRARIES})

# Alloy Components
add_library(alloy STATIC
    src/alloy/bindings.cpp
    src/alloy/crypto.cpp
    src/alloy/ipc.cpp
    src/alloy/eval.cpp
    src/alloy/streams.cpp
)
target_include_directories(alloy PUBLIC src/alloy core/mquickjs core/webview/include ${GTK3_INCLUDE_DIRS} ${WEBKIT2_INCLUDE_DIRS})
target_link_libraries(alloy PUBLIC mquickjs OpenSSL::Crypto webview_static)

# Host Executable
add_executable(alloy_host src/host.c src/bundle.c)
target_include_directories(alloy_host PRIVATE core/webview/include src/alloy core/mquickjs ${GTK3_INCLUDE_DIRS} ${WEBKIT2_INCLUDE_DIRS})
target_link_libraries(alloy_host PRIVATE alloy sqlite3 ${GTK3_LIBRARIES} ${WEBKIT2_LIBRARIES})
