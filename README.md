# AlloyScript Engine

The AlloyScript engine is a high-performance, secure JavaScript environment built using WebView as streamlined cross-platform JS runtime for desktop applications. It uses **Bun** for development and bundling, and a **C host program** for native capabilities.

## Architecture

1.  **TypeScript Library**: Provides typed APIs for SQLite, Spawn, and SecureEval.
2.  **C Host Program**: A native wrapper that initialises a [WebView](docs/webview.md) window and exposes a bridge to the JS context.
3.  **Bridge**: Communication between JS and C via `window.Alloy`.
4.  **Secure Evaluation**: `window.eval` is replaced with `secureEval` which runs [MicroQuickJS](https://github.com/bellard/mquickjs) within an OCI-compatible, chainguarded containerized Linux kernel for ultimate isolation.
5.  **SQLite Driver**: A high-performance driver with transactions, prepared statement caching, and `bigint` support.
6.  **Native GUI Framework (`alloy:gui`)**: A declarative component framework (ASX) that wraps native OS controls (Win32/Cocoa/GTK) using the Yoga layout engine.

## Security

By default, the runtime replaces the browser's `eval` with a more restricted and secure version using MicroQuickJS. The original `eval` is renamed to `_forbidden_eval` to discourage its use.

## Building

Use `bun run build` to bundle the TS source and prepare the C host for compilation.
The build script generates `build/bundle.c`, which contains the bundled JS source as a C string.

## Testing

Run tests with `bun test`. The test suite uses Bun's native features to mock the WebView environment and verify API behavior.

---

# webview

<a href="https://discord.gg/24KMecn" title="Join the chat at Discord"><img src="https://assets-global.website-files.com/6257adef93867e50d84d30e2/636e0b5061df29d55a92d945_full_logo_blurple_RGB.svg" alt="Discord" height="20" /></a>
[![Build Status](https://img.shields.io/github/actions/workflow/status/webview/webview/ci.yaml?branch=master)](https://github.com/webview/webview/actions)

A tiny cross-platform webview library for C/C++ to build modern cross-platform GUIs.

The goal of the project is to create a common HTML5 UI abstraction layer for the most widely used platforms.

It supports two-way JavaScript bindings (to call JavaScript from C/C++ and to call C/C++ from JavaScript).

> [!NOTE]
> Language binding for Go [has moved][webview_go]. Versions <= 0.1.1 are available in *this* repository.

## Platform Support

Platform | Technologies
-------- | ------------
Linux    | [GTK][gtk], [WebKitGTK][webkitgtk]
macOS    | Cocoa, [WebKit][webkit]
Windows  | [Windows API][win32-api], [WebView2][ms-webview2]

## Documentation

The most up-to-date documentation is right in the source code. Improving the documentation is a continuous effort and you are more than welcome to contribute.

## Prerequisites

Your compiler must support minimum C++11.

This project uses CMake and Ninja, and while recommended for your convenience, these tools aren't required for using the library.

### Linux and BSD

The [GTK][gtk] and [WebKitGTK][webkitgtk] libraries are required for development and distribution. You need to check your package repositories regarding which packages to install.

#### Library Dependencies

* Linux:
  * Use `pkg-config` with `--cflags` and `--libs` to get the compiler/linker options for one of these sets of modules:
    * `gtk4 webkitgtk-6.0`
    * `gtk+-3.0 webkit2gtk-4.1`
    * `gtk+-3.0 webkit2gtk-4.0`
  * Link libraries: `dl`
* macOS:
  * Link frameworks: `WebKit`
  * Link libraries: `dl`
* Windows:
  * [WebView2 from NuGet](https://www.nuget.org/packages/Microsoft.Web.WebView2).
  * Windows libraries: `advapi32 ole32 shell32 shlwapi user32 version`

[webview_go]:        https://github.com/webview/webview_go
[gtk]:               https://gtk.org/
[webkit]:            https://webkit.org/
[webkitgtk]:         https://webkitgtk.org/
[ms-webview2]:       https://developer.microsoft.com/en-us/microsoft-edge/webview2/
[win32-api]:         https://docs.microsoft.com/en-us/windows/win32/apiindex/windows-api-list
