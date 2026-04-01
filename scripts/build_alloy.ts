import { build } from "bun";
import { join } from "path";
import { writeFileSync, readFileSync, existsSync, mkdirSync } from "fs";
import { spawnSync } from "child_process";

async function buildAlloy(entryPoint: string, outputBinary: string) {
    console.log(\`Building AlloyScript: \${entryPoint} -> \${outputBinary}\`);

    const buildResult = await build({
        entrypoints: [entryPoint],
        target: "browser",
        minify: true,
    });

    if (!buildResult.success) {
        console.error("Transpilation failed:", buildResult.logs);
        process.exit(1);
    }

    const transpiledJs = await buildResult.outputs[0].text();

    const cHostTemplate = \`
#include "webview/webview.h"
#include <string>

extern "C" void webview_alloy_setup(webview_t w);

const char* embedded_js = R"javascript(
\${transpiledJs}
)javascript";

int main() {
    webview_t wv = webview_create(0, nullptr);
    webview_alloy_setup(wv);
    webview_set_title(wv, "Alloy App");
    webview_set_size(wv, 800, 600, WEBVIEW_HINT_NONE);
    webview_init(wv, embedded_js);
    webview_run(wv);
    webview_destroy(wv);
    return 0;
}
\`;

    const buildDir = "build_tmp";
    if (!existsSync(buildDir)) mkdirSync(buildDir);

    const cFilePath = join(buildDir, "host.cc");
    writeFileSync(cFilePath, cHostTemplate);

    const libPath = "build/core/libwebview.a";
    const includePath = "core/include";

    console.log("Compiling binary...");
    const gpp = spawnSync("g++", [
        cFilePath,
        "-I" + includePath,
        libPath,
        "-lutil",
        "-lsqlite3",
        "\$(pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.1)",
        "-o", outputBinary
    ], { shell: true, stdio: "inherit" });

    if (gpp.status !== 0) {
        console.error("Compilation failed.");
        process.exit(1);
    }

    console.log(\`Successfully built: \${outputBinary}\`);
}

const args = process.argv.slice(2);
if (args.length < 2) {
    console.log("Usage: bun scripts/build_alloy.ts <entry.ts> <output_binary>");
} else {
    buildAlloy(args[0], args[1]);
}
