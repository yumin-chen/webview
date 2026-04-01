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
    // Alloy.Transpiler: High-performance dual-engine redirection
    let transpiled = js;

    // 1. Browser API Forwarding (Sync Proxy to Hidden WebView)
    const browserAPIs = ["document", "window", "navigator", "localStorage", "sessionStorage", "fetch", "XMLHttpRequest", "location", "history", "screen", "alert", "prompt", "confirm"];
    const apiProxy = `
        (function() {
            const forwardToWebView = (path, args) => {
                const res_json = Alloy.webview.call(path, JSON.stringify(args));
                try { return JSON.parse(res_json); } catch(e) { return res_json; }
            };
            ${browserAPIs.map(api => `
                if (typeof ${api} === 'undefined') {
                    globalThis.${api} = new Proxy(function(){}, {
                        apply: (t, thisArg, args) => forwardToWebView('${api}', args),
                        get: (target, prop) => {
                            if (prop === 'then') return undefined;
                            return (...args) => forwardToWebView('${api}.' + String(prop), args);
                        }
                    });
                }
            `).join("\n")}
        })();
    `;

    // 2. MicroQuickJS Compatibility (Sync Promises)
    const enginePolyfills = `
        if (typeof Promise === 'undefined') {
            globalThis.Promise = function(executor) {
                let res, rej, resolved = false, rejected = false;
                executor(v => { res = v; resolved = true; }, e => { rej = e; rejected = true; });
                const p = {
                    then: (onF) => resolved ? Promise.resolve(onF(res)) : p,
                    catch: (onR) => rejected ? Promise.resolve(onR(rej)) : p
                };
                return p;
            };
            globalThis.Promise.resolve = v => (v && v.then) ? v : { then: cb => Promise.resolve(cb(v)) };
            globalThis.Promise.all = ps => ({ then: cb => cb(ps.map(p => { let r; p.then(v => r = v); return r; })) });
        }
    `;

    // 3. Native Capability Redirects (Force to Host for Perf)
    // E.g. console.log should use host stdout directly if possible, or we let webview handle it.
    // For now, we prefer host-side logic.

    return apiProxy + "\n" + enginePolyfills + "\n" + transpiled;
}
  writeFileSync("host.cc", cppHost);
}
buildExecutable(process.argv[2] || "index.ts", process.argv[3] || "app");
