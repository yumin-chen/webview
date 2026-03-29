import { build } from "bun";
import { readFileSync, writeFileSync } from "fs";
import { join } from "path";

/**
 * Builds AlloyScript source to a binary executable.
 *
 * 1. Transpiles AlloyScript using Bun.build.
 * 2. Embeds the resulting JavaScript into a C++ host program.
 * 3. Compiles the C++ host with webview and sqlite3.
 */

async function buildExecutable(entryPoint: string, outputBinary: string) {
  console.log(`Building ${entryPoint}...`);

  // 1. Transpile AlloyScript
  const result = await build({
    entrypoints: [entryPoint],
    minify: true,
    target: "browser",
  });

  if (!result.success) {
    console.error("Build failed:");
    for (const message of result.logs) {
      console.error(message);
    }
    return;
  }

  const bundledJs = await result.outputs[0].text();
  const escapedJs = JSON.stringify(bundledJs);

  // 2. Draft C++ host program
  const cppHost = `
#include "webview/webview.h"
#include <iostream>
#include <string>

int main() {
    try {
        webview::webview w(true, nullptr);
        w.set_title("AlloyScript App");
        w.set_size(800, 600, WEBVIEW_HINT_NONE);

        // The transpiled AlloyScript code
        std::string js = ${escapedJs};

        w.init(js);
        w.set_html("<!DOCTYPE html><html><head><meta charset='utf-8'></head><body><div id='app'></div></body></html>");
        w.run();
    } catch (const webview::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
`;

  writeFileSync("host.cc", cppHost);
  console.log("Generated host.cc");

  console.log("\nTo compile the final binary, run something like:");
  console.log(`c++ host.cc -std=c++11 -I./core/include -lsqlite3 $(pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.1) -o ${outputBinary}`);
}

const entry = process.argv[2] || "index.ts";
const out = process.argv[3] || "app";

buildExecutable(entry, out);
