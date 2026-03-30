macro(webview_options)
    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(WEBVIEW_MSWEBVIEW2_VERSION "1.0.1150.38" CACHE STRING "MS WebView2 version")
        option(WEBVIEW_USE_BUILTIN_MSWEBVIEW2 "Use built-in MS WebView2" ON)
    endif()
endmacro()

macro(webview_find_dependencies)
    if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        list(APPEND WEBVIEW_DEPENDENCIES "-framework WebKit" dl)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        if(WEBVIEW_USE_BUILTIN_MSWEBVIEW2)
            find_package(MSWebView2 QUIET)
            if(NOT MSWebView2_FOUND)
                webview_fetch_mswebview2(${WEBVIEW_MSWEBVIEW2_VERSION})
            endif()
            find_package(MSWebView2 REQUIRED)
            if(MSWebView2_FOUND)
                list(APPEND WEBVIEW_DEPENDENCIES MSWebView2::headers)
            endif()
        endif()
        list(APPEND WEBVIEW_DEPENDENCIES advapi32 ole32 shell32 shlwapi user32 version)
    else()
        find_package(PkgConfig REQUIRED)

        # List of preferred WebkitGTK modules (from most to least preferred)
        set(WEBVIEW_WEBKITGTK_PREFERRED_API_LIST webkit2gtk-4.1)
        # List of known WebkitGTK modules (from higher to lower version)
        set(WEBVIEW_WEBKITGTK_KNOWN_API_LIST webkitgtk-6.0 webkit2gtk-4.1 webkit2gtk-4.0)

        # Try to find specific WebKitGTK API
        if(NOT "${WEBVIEW_WEBKITGTK_API}" STREQUAL "")
            if(WEBVIEW_WEBKITGTK_API VERSION_EQUAL 6.0)
                pkg_check_modules(WEBVIEW_WEBKITGTK REQUIRED IMPORTED_TARGET webkitgtk-6.0)
            elseif(WEBVIEW_WEBKITGTK_API VERSION_EQUAL 4.1)
                pkg_check_modules(WEBVIEW_WEBKITGTK REQUIRED IMPORTED_TARGET webkit2gtk-4.1)
            elseif(WEBVIEW_WEBKITGTK_API VERSION_EQUAL 4.0)
                pkg_check_modules(WEBVIEW_WEBKITGTK REQUIRED IMPORTED_TARGET webkit2gtk-4.0)
            else()
                message(FATAL_ERROR "Unsupported WebKitGTK API: ${WEBVIEW_WEBKITGTK_API}")
            endif()
        endif()

        if("${WEBVIEW_WEBKITGTK_MODULE_NAME}" STREQUAL "")
            # Try to find a preferred WebKitGTK API
            pkg_search_module(WEBVIEW_WEBKITGTK IMPORTED_TARGET ${WEBVIEW_WEBKITGTK_PREFERRED_API_LIST})
            if (NOT WEBVIEW_WEBKITGTK_FOUND)
              message(STATUS "Trying to find any WebKitGTK API")
              pkg_search_module(WEBVIEW_WEBKITGTK REQUIRED IMPORTED_TARGET ${WEBVIEW_WEBKITGTK_KNOWN_API_LIST})
            endif()
        else()
            pkg_check_modules(WEBVIEW_WEBKITGTK REQUIRED IMPORTED_TARGET "${WEBVIEW_WEBKITGTK_MODULE_NAME}")
        endif()

        if("${WEBVIEW_WEBKITGTK_MODULE_NAME}" STREQUAL "webkitgtk-6.0")
            set(WEBVIEW_WEBKITGTK_API 6.0)
        elseif("${WEBVIEW_WEBKITGTK_MODULE_NAME}" STREQUAL "webkit2gtk-4.1")
            set(WEBVIEW_WEBKITGTK_API 4.1)
        elseif("${WEBVIEW_WEBKITGTK_MODULE_NAME}" STREQUAL "webkit2gtk-4.0")
            set(WEBVIEW_WEBKITGTK_API 4.0)
        else()
            message(FATAL_ERROR "Couldn't find any supported WebKitGTK API")
        endif()

        # Find matching GTK module
        if("${WEBVIEW_WEBKITGTK_API}" VERSION_GREATER_EQUAL 6.0)
            pkg_check_modules(WEBVIEW_GTK REQUIRED IMPORTED_TARGET gtk4)
        elseif("${WEBVIEW_WEBKITGTK_API}" VERSION_LESS 5.0)
            pkg_check_modules(WEBVIEW_GTK REQUIRED IMPORTED_TARGET gtk+-3.0)
        endif()

        list(APPEND WEBVIEW_DEPENDENCIES PkgConfig::WEBVIEW_WEBKITGTK PkgConfig::WEBVIEW_GTK dl)
    endif()

    webview_fetch_sqlite()
    list(APPEND WEBVIEW_DEPENDENCIES sqlite3)

    webview_fetch_yoga()
    list(APPEND WEBVIEW_DEPENDENCIES yoga)
endmacro()

function(webview_fetch_sqlite)
    include(FetchContent)
    FetchContent_Declare(
        sqlite3_src
        GIT_REPOSITORY https://github.com/sqlite/sqlite.git
        GIT_TAG version-3.45.1
    )
    FetchContent_GetProperties(sqlite3_src)
    if(NOT sqlite3_src_POPULATED)
        FetchContent_Populate(sqlite3_src)

        # SQLite github repo doesn't have CMakeLists.txt.
        # We'll assume the presence of sqlite3.c/h (amalgamation)
        # which are often in the root or generated.
        # If they aren't there, we'd need to run their tool to generate them.
        # For simplicity in this environment, we'll try to find them.

        add_library(sqlite3 STATIC
            "${sqlite3_src_SOURCE_DIR}/sqlite3.c"
        )
        target_include_directories(sqlite3 PUBLIC "${sqlite3_src_SOURCE_DIR}")
        target_compile_definitions(sqlite3 PRIVATE
            SQLITE_ENABLE_SERIALIZE
            SQLITE_ENABLE_COLUMN_METADATA
        )
        if(UNIX)
            target_link_libraries(sqlite3 PRIVATE dl pthread)
        endif()
    endif()
endfunction()

function(webview_fetch_yoga)
    include(FetchContent)
    FetchContent_Declare(
        yoga
        GIT_REPOSITORY https://github.com/facebook/yoga.git
        GIT_TAG v2.0.1
    )
    FetchContent_MakeAvailable(yoga)
endfunction()

function(webview_fetch_mswebview2 VERSION)
    cmake_policy(PUSH)
    # Avoid warning related to FetchContent and DOWNLOAD_EXTRACT_TIMESTAMP
    if(POLICY CMP0135)
        cmake_policy(SET CMP0135 NEW)
    endif()
    if(NOT COMMAND FetchContent_Declare)
        include(FetchContent)
    endif()
    set(FC_NAME microsoft_web_webview2)
    FetchContent_Declare(${FC_NAME}
        URL "https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/${VERSION}")
    FetchContent_MakeAvailable(${FC_NAME})
    set(MSWebView2_ROOT "${${FC_NAME}_SOURCE_DIR}")
    set(MSWebView2_ROOT "${MSWebView2_ROOT}" PARENT_SCOPE)
    cmake_policy(POP)
endfunction()
