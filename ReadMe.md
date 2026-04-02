# AlloyScript Engine

The AlloyScript engine is a high-performance, secure JavaScript environment built using WebView as streamlined cross-platform JS runtime for desktop applications. It uses **Bun** for development and bundling, and a **C host program** for native capabilities.

## Architecture

1.  **TypeScript Library**: Provides typed APIs for SQLite, Spawn, and SecureEval.
2.  **C Host Program**: A native wrapper that initialises a [WebView](docs/webview.md) window and exposes a bridge to the JS context.
3.  **Bridge**: Communication between JS and C via `window.Alloy`. All communication is transparently encrypted using **End-to-End Encryption (E2EE)** with AES-256-GCM.
4.  **Secure Evaluation**: `window.eval` is replaced with `secureEval` which runs [MicroQuickJS](https://github.com/bellard/mquickjs) for ultimate isolation. The main C process executes the MicroQuickJS runtime, while the WebView remains hidden and is used only for browser-specific APIs.
5.  **SQLite Driver**: A high-performance driver with transactions, prepared statement caching, and `bigint` support.
6.  **Native GUI Framework (`alloy:gui`)**: A declarative component framework (ASX) that wraps native OS controls (Win32/Cocoa/GTK) using the Yoga layout engine.

## Security

The AlloyScript engine implements **Defense in Depth** by treating the WebView process as inherently insecure:
- **E2EE IPC**: All data exchanged between the Main C Process and the WebView is encrypted using AES-256-GCM, with keys established via X25519 key exchange. This prevents a compromised OS or WebView from intercepting sensitive API calls.
- **Isolated Execution**: Application logic runs within MicroQuickJS in the secure C process. The WebView is hidden and restricted.
- **Identity-based Bindings**: Identity is managed via DID-based key pairs.
- **Secure Eval**: `window.eval` is replaced with `secureEval` using the MicroQuickJS engine. The original `eval` is renamed to `_forbidden_eval`.

## Building

Use `bun run build` to bundle the TS source and prepare the C host for compilation.
The build script generates `build/bundle.c`, which contains the bundled JS source as a C string.

## Testing

Run tests with `bun test`. The test suite uses Bun's native features to mock the WebView environment and verify API behavior.
