import { build } from "bun";
import { writeFileSync, readFileSync, mkdirSync } from "fs";
import { join } from "path";

const entrypoint = process.argv[2] || "index.ts";
const outDir = "dist";

async function main() {
  console.log(`Building AlloyScript project: @alloyscript/runtime`);
  console.log(`Entrypoint: ${entrypoint}`);

  try {
    mkdirSync(outDir, { recursive: true });
  } catch (e) {}

  const result = await build({
    entrypoints: [entrypoint],
    outdir: outDir,
    target: "browser",
    minify: false, // Keep it readable for debugging
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

  if (!result.success) {
    console.error("Build failed:", result.logs);
    process.exit(1);
  }

  const jsContent = readFileSync(join(outDir, "index.js"), "utf8");
  const escapedJs = JSON.stringify(jsContent);

  const cHostTemplate = `
#include "webview/webview.h"
#include <iostream>
#include <string>

const char* bundled_js = ${escapedJs};

int main() {
    try {
        webview::webview w(true, nullptr);
        w.set_title("AlloyScript Runtime Host");
        w.set_size(1024, 768, WEBVIEW_HINT_NONE);
        // Initialization script to set up bindings
        w.init(bundled_js);
        w.set_html("<html><head><style>body { font-family: sans-serif; }</style></head><body><h1>AlloyScript App</h1><p>The runtime is initialized. Inspect the console for logs.</p><script>" + std::string(bundled_js) + "</script></body></html>");
        w.run();
    } catch (const webview::exception &e) {
        std::cerr << "Webview Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
`;

  writeFileSync("host.cc", cHostTemplate);
  console.log("Success! Generated host.cc with embedded JavaScript bundle.");
  console.log("Next step: Compile host.cc with your platform's C++ compiler linking the webview library.");
}

main();
