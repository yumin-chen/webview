# @alloyscript/runtime Architecture & Implementation

The AlloyScript runtime is a high-performance, secure JavaScript environment built for WebView applications. It uses **Bun** for development and bundling, and a **C host program** for native capabilities.

## Architecture

1.  **TypeScript Library**: Provides typed APIs for SQLite, Process Spawning, and Secure Evaluation.
2.  **C Host Program (`src/host.c`)**: A native wrapper that initializes a WebView window and exposes a bridge to the JS context via `window.Alloy`.
3.  **Bridge**: Synchronous and asynchronous communication between JS and C via `window.Alloy` and `webview_bind`.
4.  **Secure Evaluation**: `globalThis.eval` is replaced with `secureEval` which runs [MicroQuickJS](https://github.com/bellard/mquickjs) within a containerized Linux kernel for ultimate isolation.
5.  **SQLite Driver (`src/sqlite.ts`)**: A high-performance driver featuring transactions (`deferred`, `immediate`, `exclusive`), prepared statement caching, `bigint` (signed 64-bit) validation, and `.as(Class)` mapping.
6.  **Native GUI Framework (`alloy:gui`)**: A declarative component framework (ASX/JSX) that wraps native OS controls (Win32/Cocoa/GTK) using the Yoga layout engine for flexbox-based layout.

## GUI Component Framework

The framework includes 45+ modular components in `src/gui/components/`, re-exported via `src/gui/components.ts`. Each component maps to a native control in `src/gui/alloy.c`:

- **Layout**: `VStack`, `HStack`, `ScrollView`, `GroupBox`, `Splitter`
- **Input**: `Button`, `TextField`, `TextArea`, `CheckBox`, `RadioButton`, `ComboBox`, `Slider`, `Spinner`, `DatePicker`, `TimePicker`, `ColorPicker`, `Switch`, `Rating`
- **Display**: `Label`, `Image`, `Icon`, `ProgressBar`, `LoadingSpinner`, `Badge`, `Card`, `Divider`, `Tooltip`, `Badge`, `Link`, `Chip`
- **Navigation/Menus**: `Menu`, `MenuBar`, `Toolbar`, `ContextMenu`, `Accordion`, `TabView`, `TreeView`, `ListView`
- **Dialogs**: `Dialog`, `FileDialog`, `Popover`, `StatusBar`
- **Rich Content**: `WebView`, `RichTextEditor`, `CodeEditor`

## Native C Bindings

The native implementation (`src/gui/alloy.c`) provides:
- **Win32 Backend**: Uses `CreateWindowExW` with standard classes (`BUTTON`, `EDIT`, `STATIC`, etc.).
- **GTK Backend**: Uses `gtk_button_new`, `gtk_entry_new`, etc., and manages widgets via `GtkContainer`.
- **Layout**: Uses the Yoga flexbox engine (`YGNodeRef`) to calculate component positions.
- **Dispatch**: Thread-safe UI updates via `alloy_dispatch`.

## Security Model

By default, the runtime replaces the browser's `eval` with a more restricted and secure version using MicroQuickJS. The original `eval` is renamed to `_forbidden_eval` in the bridge to prevent accidental usage.

## Build System (`scripts/build.ts`)

Use `bun run build` to:
1. Bundle the TypeScript source using `Bun.build`.
2. Generate `build/bundle.c` which contains the minified JS source as an escaped C string.
3. Compile the C host program using `gcc` (linking against `sqlite3`, `mquickjs`, `webview`, and platform GUI libraries).

## Testing (`tests/`)

Run tests with `bun test`.
- `tests/components/`: Individual unit tests for all 45+ GUI components.
- `tests/sqlite.test.ts`: Comprehensive SQLite engine verification.
- `tests/spawn.test.ts`: Process management and SecureEval tests.
- `tests/e2e.test.ts`: Full application lifecycle and JS bridge routing simulation.
