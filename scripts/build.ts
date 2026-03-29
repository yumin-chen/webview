import { build } from "bun";
import { writeFileSync, readFileSync } from "fs";
import { join } from "path";

const entrypoint = process.argv[2] || "index.ts";
const outDir = "dist";

async function main() {
  console.log(`Building AlloyScript: ${entrypoint}`);

  const result = await build({
    entrypoints: [entrypoint],
    outdir: outDir,
    target: "browser",
    minify: true,
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
        w.set_title("AlloyScript App");
        w.set_size(800, 600, WEBVIEW_HINT_NONE);
        w.init(bundled_js);
        w.set_html("<html><body><script>" + std::string(bundled_js) + "</script></body></html>");
        w.run();
    } catch (const webview::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
`;

  writeFileSync("host.cc", cHostTemplate);
  console.log("Generated host.cc with embedded JavaScript.");
}

main();
