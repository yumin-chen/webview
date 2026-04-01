# @alloyscript/engine Dual-Engine Architecture

This project implements a secure, high-performance runtime for AlloyScript using a **Dual-Engine Architecture**.

## Architecture Overview

The system consists of two primary execution environments orchestrated by a native C host:

1.  **Safe Host Process (MicroQuickJS)**:
    - Primary engine for executing AlloyScript logic.
    - Runs in a secure, isolated environment.
    - Handles sensitive operations like File I/O, SQLite, and Process Management.
    - Uses MicroQuickJS compiled to native machine code or WASM.

2.  **Unsafe WebView Process (Browser Capacities)**:
    - Hidden by default to reinforce defense-in-depth.
    - Acts purely as a **Capacities Provider**.
    - Exposes Browser-native APIs (GPU, Window, Document) to the Safe Host.
    - Logic execution here is considered untrusted.

## ABI Boundary & Polyfilling

The C host manages a high-speed ABI bridge between the two engines:

- **Automatic Polyfilling**: MicroQuickJS is automatically polyfilled with standard Browser APIs. When AlloyScript code accesses `window` or `document`, the request is transparently delegated to the WebView via a secure proxy.
- **IPC Encryption**: Messages crossing the ABI boundary are encrypted using a per-session E2E encryption shim to prevent manipulation by malicious scripts in the WebView.
- **Global Bindings**: Critical APIs are bound to the global scope (`bind_global`) instead of being attached to the `window` object, protecting them from prototype pollution.

## Transpiler & Bytecode

The `Alloy.Transpiler` uses the MicroQuickJS parser to:
- **Validate Syntax**: Engine-level parsing of JS and TS.
- **Generate Bytecode**: Compile code to internal MicroQuickJS bytecode for efficient distribution.
- **Reconstruction**: Restore JS code from bytecode when targeting Node.js.
- **Async Polyfilling**: Automatically inject polyfills to forward `async/await` operations to the WebView capacities provider.

## Security Model

By treating the WebView as an inherently hostile layer, we achieve "Secure by Design":
- Sensitive data never resides in the WebView's heap.
- UI manipulation is restricted to high-level GUI components.
- The browser runtime is used only for what it does best: rendering and specific web-native APIs.
