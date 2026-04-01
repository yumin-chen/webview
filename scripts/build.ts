import { build } from "bun";
import { readFileSync, writeFileSync } from "fs";
import { join } from "path";

async function main() {
    const entrypoint = process.argv[2] || "index.ts";
    const outDir = "dist";

    console.log(`Building ${entrypoint}...`);

    const result = await build({
        entrypoints: [entrypoint],
        outdir: outDir,
        target: "browser",
        minify: true,
    });

    if (!result.success) {
        console.error("Build failed:", result.logs);
        process.exit(1);
    }

    const transpiledJS = readFileSync(join(outDir, "index.js"), "utf8");

    const hostProgram = `
#include "webview/webview.h"
#include <iostream>
#include <string>

const char* transpiled_js = R"js(
${transpiledJS}
)js";

int main() {
    try {
        webview::webview w(true, nullptr);
        w.set_title("AlloyScript Runtime");
        w.set_size(1024, 768, WEBVIEW_HINT_NONE);
        w.init(transpiled_js);
        w.run();
    } catch (const webview::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
`;

    writeFileSync("host_program.cc", hostProgram);
    console.log("Host program generated: host_program.cc");
}

main();
