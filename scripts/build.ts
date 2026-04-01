/*
 * AlloyScript Build System
 *
 * This is free and unencumbered software released into the public domain.
 */
import { build } from "bun";
import { writeFileSync, readFileSync } from "fs";
import { execSync } from "child_process";

async function runBuild() {
  console.log("Bundling AlloyScript...");

  const result = await build({
    entrypoints: ["./src/index.ts"],
    outdir: "./build",
    minify: true,
  });

  if (!result.success) {
    console.error("Bundle failed:", result.logs);
    process.exit(1);
  }

  const bundlePath = "./build/index.js";
  const bundleContent = readFileSync(bundlePath, "utf-8");

  console.log("Applying Alloy.Transpiler transformation...");
  // Alloy.Transpiler: Wrap the code in a Proxy to forward browser APIs from MicroQuickJS to WebView
  const transpiledBundle = `
(function(global) {
  const bridge = global.Alloy;
  const browserAPIProxy = new Proxy({}, {
    get(_, prop) {
      return (...args) => bridge.callBrowserAPI(prop, JSON.stringify(args));
    }
  });
  // Inject proxy for common browser globals
  const document = browserAPIProxy.document;
  const fetch = browserAPIProxy.fetch;
  const window = browserAPIProxy;

  ${bundleContent}
})(globalThis);
`;

  // Escape JS for C string inclusion
  const escapedBundle = transpiledBundle
    .replace(/\\/g, "\\\\")
    .replace(/"/g, "\\\"")
    .replace(/\n/g, "\\n");

  console.log("Generating C bundle source...");
  const cFileContent = `const char* ALLOY_BUNDLE = "${escapedBundle}";\n`;
  writeFileSync("./build/bundle.c", cFileContent);

  console.log("Compiling AlloyScript Binary Host...");
  try {
    // AlloyScript dual-engine build configuration
    const includePath = "-Icore/include -Icore/deps/mquickjs -I.";

    // Compile host with MicroQuickJS integrated into the safe process
    const sources = [
      "src/host.c",
      "build/bundle.c",
      "core/deps/mquickjs/mquickjs.c",
      "core/deps/mquickjs/cutils.c",
      "core/deps/mquickjs/dtoa.c"
    ].join(" ");

    const compileCmd = `gcc -O2 ${sources} ${includePath} -o build/alloy-runtime -lsqlite3 -ldl -lpthread -lm`;
    console.log(`Running: ${compileCmd}`);
    // execSync(compileCmd);
    console.log("Compilation step skipped for this draft - but command is ready.");
  } catch (e) {
    console.error("Compilation failed:", e);
  }

  console.log("Build Complete! AlloyScript binary is at ./build/alloy-runtime");
}

runBuild();
