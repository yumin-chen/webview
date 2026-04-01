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
    // Alloy.Transpiler: Advanced Browser API forwarding and MicroQuickJS polyfills
    let transpiled = js;

    // 1. Browser API Proxying (Forward to WebView)
    const browserAPIs = ["document", "window", "navigator", "localStorage", "fetch", "XMLHttpRequest", "location", "history", "screen"];
    const apiProxy = `
        (function() {
            const forwardToWebView = (path, args) => {
                const res_json = Alloy.webview.call(path, args); // Sync call from MicroQuickJS to WebView
                try { return JSON.parse(res_json); } catch(e) { return res_json; }
            };
            ${browserAPIs.map(api => `
                if (typeof ${api} === 'undefined') {
                    globalThis.${api} = new Proxy({}, {
                        get: (target, prop) => {
                            if (prop === 'then') return undefined;
                            return (...args) => forwardToWebView('${api}.' + String(prop), args);
                        }
                    });
                }
            `).join("\n")}
        })();
    `;

    // 2. MicroQuickJS Polyfills (Async/Await & Promises)
    const enginePolyfills = `
        if (typeof Promise === 'undefined') {
            globalThis.Promise = function(executor) {
                let resolveValue, rejectValue, isResolved = false, isRejected = false;
                executor(v => { resolveValue = v; isResolved = true; }, e => { rejectValue = e; isRejected = true; });
                const p = {
                    then: function(onFulfilled) { if (isResolved) return Promise.resolve(onFulfilled(resolveValue)); return p; },
                    catch: function(onRejected) { if (isRejected) return Promise.resolve(onRejected(rejectValue)); return p; }
                };
                return p;
            };
            globalThis.Promise.resolve = v => {
                if (v && v.then) return v;
                return { then: cb => Promise.resolve(cb(v)), catch: cb => this };
            };
            globalThis.Promise.all = (promises) => {
                return { then: cb => cb(promises.map(p => { let r; p.then(v => r = v); return r; })) };
            };
        }
    `;

    transpiled = apiProxy + "\n" + enginePolyfills + "\n" + transpiled;

    return transpiled;
}
  writeFileSync("host.cc", cppHost);
}
buildExecutable(process.argv[2] || "index.ts", process.argv[3] || "app");
