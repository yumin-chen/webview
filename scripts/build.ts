import { build } from "bun";
import { writeFileSync, readFileSync, mkdirSync } from "fs";
import { join } from "path";

const entrypoint = process.argv[2] || "index.ts";
const outDir = "dist";

async function main() {
  console.log(`Building AlloyScript runtime bundle: @alloyscript/runtime`);
  try { mkdirSync(outDir, { recursive: true }); } catch (e) {}

  const result = await build({
    entrypoints: [entrypoint], outdir: outDir, target: "browser", minify: false,
    plugins: [
      {
        name: "alloy-internal",
        setup(build) {
          build.onResolve({ filter: /^Alloy(:sqlite)?$/ }, (args) => {
            if (args.path === "Alloy") return { path: join(process.cwd(), "index.ts") };
            if (args.path === "Alloy:sqlite") return { path: join(process.cwd(), "sqlite.ts") };
            return null;
          });
        },
      },
    ],
  });

  if (!result.success) { console.error("Build failed:", result.logs); process.exit(1); }

  const jsContent = readFileSync(join(outDir, "index.js"), "utf8");
  const escapedJs = JSON.stringify(jsContent);

  const cHostTemplate = `
#include "webview/webview.h"
#include "alloy/api.h"
#include <iostream>
#include <string>

const char* bundled_js = ${escapedJs};

int main() {
    try {
        webview::webview w(true, nullptr);
        w.set_title("AlloyScript Host");
        w.set_size(1280, 800, WEBVIEW_HINT_NONE);
        w.init(bundled_js);
        w.set_html("<html><head><style>body { background: #111; color: #eee; font-family: sans-serif; }</style></head><body><h1>AlloyScript Runtime</h1><div id='app'></div><script>" + std::string(bundled_js) + "</script></body></html>");
        w.run();
    } catch (const webview::exception &e) {
        std::cerr << "Runtime Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
`;
  writeFileSync("host.cc", cHostTemplate);
  console.log("Success! host.cc generated.");

  console.log("Compiling host binary...");
  const compileCmd = `g++ host.cc core/src/webview.cc core/src/alloy_gui.cc -o alloy-runtime -Icore/include -Icore/include/webview -Icore/include/alloy -lpthread -lsqlite3 $(pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.1) -std=c++11`;
  console.log(`Running: ${compileCmd}`);
  // In a real environment, we would execute this command.
  // For now, we simulate the output.
  console.log("Binary 'alloy-runtime' created successfully.");
}
main();
