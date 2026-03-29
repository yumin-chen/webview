# AlloyScript

The AlloyScript runtime is a high-performance, secure JavaScript environment built for WebView applications. It uses **Bun** for development and bundling, and a **C host program** for native capabilities.

## Architecture

1.  **TypeScript Library**: Provides typed APIs for SQLite, Spawn, and SecureEval.
2.  **C Host Program**: A native wrapper that initializes a WebView window and exposes a bridge to the JS context.
3.  **Bridge**: Communication between JS and C via `window.Alloy`.
4.  **Secure Evaluation**: `window.eval` is replaced with `secureEval` which runs [MicroQuickJS](https://github.com/bellard/mquickjs) within an OCI-compatible, chainguarded containerized Linux kernel for ultimate isolation.
5.  **SQLite Driver**: A high-performance driver with transactions, prepared statement caching, and `bigint` support.

## Security

By default, the runtime replaces the browser's `eval` with a more restricted and secure version using MicroQuickJS. The original `eval` is renamed to `_forbidden_eval` to discourage its use.

## Building

Use `bun run build` to bundle the TS source and prepare the C host for compilation.
The build script generates `build/bundle.c`, which contains the bundled JS source as a C string.

## Testing

Run tests with `bun test`. The test suite uses Bun's native features to mock the WebView environment and verify API behavior.
