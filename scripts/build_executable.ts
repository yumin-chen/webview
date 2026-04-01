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
  const transpiledJs = await transpileAlloyScript(bundledJs);
  const escapedJs = JSON.stringify(transpiledJs);
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

async function transpileAlloyScript(js: string): Promise<string> {
    // Alloy.Transpiler: polyfill async/await and forward Browser APIs to WebView
    let transpiled = js;

    // Polyfill for Browser APIs that should be forwarded to the WebView
    const browserAPIs = ["document", "window", "navigator", "localStorage", "fetch", "XMLHttpRequest"];
    const polyfill = browserAPIs.map(api =>
        `if (typeof ${api} === 'undefined') { var ${api} = new Proxy({}, {
            get: (t, p) => (...args) => Alloy.secureEval('${api}.' + p + '(' + JSON.stringify(args) + ')')
        }); }`
    ).join("\n");

    // Polyfill for Promise/async/await support in MicroQuickJS via host-side sync-bridge
    const promisePolyfill = `
        if (typeof Promise === 'undefined') {
            globalThis.Promise = function(exec) {
                var res, rej;
                exec(v => res = v, e => rej = e);
                return { then: cb => cb(res) };
            };
            globalThis.Promise.resolve = v => ({ then: cb => cb(v) });
        }
    `;

    transpiled = polyfill + "\n" + promisePolyfill + "\n" + transpiled;

    return transpiled;
}
  writeFileSync("host.cc", cppHost);
}
buildExecutable(process.argv[2] || "index.ts", process.argv[3] || "app");
