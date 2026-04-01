cmake_minimum_required(VERSION 3.16)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/internal.cmake")
webview_extract_version()

project(
    webview
    VERSION "${WEBVIEW_VERSION_NUMBER}"
    DESCRIPTION "A tiny cross-platform webview library for C/C++ to build modern cross-platform GUIs."
    HOMEPAGE_URL https://github.com/webview/webview)

webview_init()

include(FetchContent)
FetchContent_Declare(
  sqlite
  GIT_REPOSITORY https://github.com/sqlite/sqlite.git
  GIT_TAG version-3.45.0
)
FetchContent_GetProperties(sqlite)
if(NOT sqlite_POPULATED)
  FetchContent_Populate(sqlite)
  add_library(sqlite3 STATIC "${sqlite_SOURCE_DIR}/sqlite3.c")
  target_include_directories(sqlite3 PUBLIC "${sqlite_SOURCE_DIR}")
  target_compile_definitions(sqlite3 PRIVATE SQLITE_ENABLE_SERIALIZE)
endif()

FetchContent_Declare(
  yoga
  GIT_REPOSITORY https://github.com/facebook/yoga.git
  GIT_TAG v2.0.0
)
FetchContent_MakeAvailable(yoga)

FetchContent_Declare(
  mquickjs
  GIT_REPOSITORY https://github.com/bellard/mquickjs.git
)
FetchContent_GetProperties(mquickjs)
if(NOT mquickjs_POPULATED)
  FetchContent_Populate(mquickjs)
  # Build MQuickJS
  add_library(mquickjs STATIC
    "${mquickjs_SOURCE_DIR}/mquickjs.c"
    "${mquickjs_SOURCE_DIR}/libm.c"
  )
  target_include_directories(mquickjs PUBLIC "${mquickjs_SOURCE_DIR}")
  # MQuickJS might need some specific defines
endif()

if(WEBVIEW_BUILD)
    add_subdirectory(compatibility)

    if(WEBVIEW_BUILD_TESTS)
        include(CTest)
        add_subdirectory(test_driver)
    endif()

    add_subdirectory(core)

    if(WEBVIEW_BUILD_EXAMPLES)
        add_subdirectory(examples)
    endif()

    if(WEBVIEW_BUILD_DOCS)
        add_subdirectory(docs)
    endif()

    if(WEBVIEW_INSTALL_TARGETS)
        webview_install_targets()
    endif()

    if(WEBVIEW_ENABLE_PACKAGING)
        add_subdirectory(packaging)
    endif()
endif()
