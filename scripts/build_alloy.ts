import { build } from "bun";
import { writeFileSync } from "node:fs";

async function buildAlloy(sourceFile: string, outputFile: string) {
    console.log(`Building ${sourceFile}...`);

    const result = await build({
        entrypoints: [sourceFile],
        minify: true,
        target: "browser",
    });

    if (!result.success) {
        console.error("Build failed:", result.logs);
        process.exit(1);
    }

    const transpiledJs = await result.outputs[0].text();

    const cppTemplate = `
#include "webview/webview.h"
#include <string>

int main() {
    webview::webview w(false, nullptr);
    w.set_title("AlloyScript App");
    w.set_size(1024, 768, WEBVIEW_HINT_NONE);
    std::string js = R"js(${transpiledJs})js";
    w.init(js);
    w.set_html("<!DOCTYPE html><html><body><div id='app'></div></body></html>");
    w.run();
    return 0;
}
`;

    const tempCpp = "temp_app.cpp";
    writeFileSync(tempCpp, cppTemplate);
    console.log(`Embedded source into ${tempCpp}. Use CMake to build the final binary.`);
}

const source = process.argv[2] || "index.ts";
const output = process.argv[3] || "app";
buildAlloy(source, output);
