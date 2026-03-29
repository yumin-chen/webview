import { build } from "bun";
import { spawnSync } from "child_process";
import { writeFileSync, readFileSync, existsSync, mkdirSync, unlinkSync } from "fs";
import { join, basename, resolve } from "path";

async function run() {
    const entry = process.argv[2] || "index.ts";
    const output = process.argv[3] || "app";

    if (!existsSync(entry)) {
        console.error(`Entry file ${entry} not found.`);
        process.exit(1);
    }

    console.log("Building MetaScript bundle...");

    // Create a temporary entry point that imports the runtime and the user code
    const tempEntry = ".metascript_entry.ts";
    const entryRel = "./" + basename(entry);
    const runtimeRel = "./src/runtime.ts";

    writeFileSync(tempEntry, `
import "${runtimeRel}";
import userCode from "${entryRel}";
(window as any).defaultExport = userCode;
`);

    // Bundle everything
    const result = await build({
        entrypoints: [tempEntry],
        minify: true,
        outdir: "dist",
        naming: "bundle.js"
    });

    unlinkSync(tempEntry);

    if (!result.success) {
        console.error("Bundle failed", result.logs);
        process.exit(1);
    }

    const bundledJs = readFileSync("dist/bundle.js", "utf-8");

    // Generate C header
    console.log("Generating host bundle...");
    const escapedJs = JSON.stringify(bundledJs);
    const headerContent = `#ifndef METASCRIPT_BUNDLE_H\n#define METASCRIPT_BUNDLE_H\nstatic const char* METASCRIPT_BUNDLE = ${escapedJs};\n#endif\n`;

    if (!existsSync("dist")) mkdirSync("dist");
    writeFileSync("dist/bundle.h", headerContent);

    // Compile C++ host
    console.log("Compiling binary...");

    let cflags: string[] = [];
    let libs: string[] = [];

    try {
        cflags = spawnSync("pkg-config", ["--cflags", "gtk+-3.0", "webkit2gtk-4.1"]).stdout.toString().trim().split(/\s+/);
        libs = spawnSync("pkg-config", ["--libs", "gtk+-3.0", "webkit2gtk-4.1"]).stdout.toString().trim().split(/\s+/);
    } catch (e) {
        console.error("pkg-config failed, trying default paths...");
    }

    const compileArgs = [
        "-std=c++11",
        "-Icore/include",
        "-Idist",
        "src/host.cpp",
        "core/src/webview.cc",
        "-o", output,
        "-lutil",
        ...cflags.filter(s => s.length > 0),
        ...libs.filter(s => s.length > 0)
    ];

    const compile = spawnSync("c++", compileArgs);

    if (compile.status !== 0) {
        console.error("Compilation failed");
        console.error(compile.stderr.toString());
        process.exit(1);
    }

    console.log(`Success! Binary created: ${output}`);
}

run();
