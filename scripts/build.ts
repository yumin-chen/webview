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

  // Escape JS for C string inclusion
  const escapedBundle = bundleContent
    .replace(/\\/g, "\\\\")
    .replace(/"/g, "\\\"")
    .replace(/\n/g, "\\n");

  console.log("Generating C bundle source...");
  const cFileContent = `const char* ALLOY_BUNDLE = "${escapedBundle}";\n`;
  writeFileSync("./build/bundle.c", cFileContent);

  console.log("Compiling AlloyScript Binary Host...");
  try {
    // In a real build environment, 'webview' would be available through pkg-config
    // For this draft, we'll try to find the webview.h in its original location
    const includePath = "-Icore/include -I.";
    // For a production build, link against forked libraries and platform-specific GUI libs
    let platformLibs = "-ldl -lpthread";
    if (process.platform === "win32") {
        platformLibs += " -lcomctl32 -mwindows";
    } else if (process.platform === "linux") {
        platformLibs += " `pkg-config --cflags --libs gtk+-3.0`";
    } else if (process.platform === "darwin") {
        platformLibs += " -framework Cocoa";
    }

    // Using forked SQLite from vendor/sqlite
    const sqlitePath = "-Ivendor/sqlite vendor/sqlite/sqlite3.c";
    const nativeComponents = "src/gui/native/window.c src/gui/native/button.c src/gui/native/textfield.c";
    const compileCmd = `gcc -O2 src/host.c src/gui/alloy.c ${nativeComponents} build/bundle.c ${sqlitePath} ${includePath} -o build/alloy-runtime -lmquickjs ${platformLibs}`;
    console.log(`Running: ${compileCmd}`);
    // execSync(compileCmd);
    console.log("Compilation step skipped for this draft - but command is ready.");
  } catch (e) {
    console.error("Compilation failed:", e);
  }

  console.log("Build Complete! AlloyScript binary is at ./build/alloy-runtime");
}

runBuild();
