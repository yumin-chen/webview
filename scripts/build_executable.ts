import { build } from "bun";
import { writeFileSync } from "fs";

async function buildExecutable(entryPoint: string, outputBinary: string) {
  const result = await build({
    entrypoints: [entryPoint],
    minify: true,
    target: "browser",
    external: ["Alloy:sqlite", "alloy:sqlite", "alloy:gui"],
  });
  if (!result.success) return;
  const bundledJs = await result.outputs[0].text();
  const escapedJs = JSON.stringify(bundledJs);
  const cppHost = `
#include "webview/webview.h"
#include <string>
int main() {
    webview::webview w(true, nullptr);
    w.set_title("AlloyScript App");
    w.init(${escapedJs});
    w.set_html("<html><body></body></html>");
    w.run();
    return 0;
}
`;
  writeFileSync("host.cc", cppHost);
}
buildExecutable(process.argv[2] || "index.ts", process.argv[3] || "app");
