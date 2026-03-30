import { build } from "bun";
import { spawnSync } from "child_process";
import { writeFileSync, readFileSync, existsSync, mkdirSync, unlinkSync } from "fs";
import { join, basename } from "path";

async function run() {
    const entry = process.argv[2] || "index.ts";
    const output = process.argv[3] || "app";

    if (!existsSync(entry)) {
        console.error(`Entry file ${entry} not found.`);
        process.exit(1);
    }

    console.log("Building AlloyScript bundle...");

    const tempEntry = ".alloyscript_entry.ts";
    const entryRel = "./" + basename(entry);
    const runtimeRel = "./src/runtime.ts";

    writeFileSync(tempEntry, `
import "${runtimeRel}";
import userCode from "${entryRel}";
(window as any).defaultExport = userCode;
`);

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

    console.log("Generating host bundle...");
    const escapedJs = JSON.stringify(bundledJs);
    const headerContent = `#ifndef ALLOYSCRIPT_BUNDLE_H\n#define ALLOYSCRIPT_BUNDLE_H\nstatic const char* ALLOYSCRIPT_BUNDLE = ${escapedJs};\n#endif\n`;

    if (!existsSync("dist")) mkdirSync("dist");
    writeFileSync("dist/bundle.h", headerContent);

    console.log("Compiling binary...");

    let cflags: string[] = [];
    let libs: string[] = [];

    try {
        cflags = spawnSync("pkg-config", ["--cflags", "gtk+-3.0", "webkit2gtk-4.1"]).stdout.toString().trim().split(/\s+/);
        libs = spawnSync("pkg-config", ["--libs", "gtk+-3.0", "webkit2gtk-4.1"]).stdout.toString().trim().split(/\s+/);
    } catch (e) {}

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
