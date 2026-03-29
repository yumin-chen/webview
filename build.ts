import { build } from "bun";
import { spawnSync } from "child_process";
import { writeFileSync, readFileSync, existsSync, mkdirSync } from "fs";
import { join, basename } from "path";

async function run() {
    const entry = process.argv[2] || "index.ts";
    const output = process.argv[3] || "app";

    if (!existsSync(entry)) {
        console.error(`Entry file ${entry} not found.`);
        process.exit(1);
    }

    console.log("Building MetaScript bundle...");

    // Bundle runtime + user code
    const result = await build({
        entrypoints: ["src/runtime.ts", entry],
        minify: true,
        outdir: "dist/bundle",
        naming: "[name].js"
    });

    if (!result.success) {
        console.error("Bundle failed", result.logs);
        process.exit(1);
    }

    // Combine runtime and entry into one script
    const runtimeJs = readFileSync("dist/bundle/runtime.js", "utf-8");
    const entryBase = basename(entry).replace(/\.[^/.]+$/, "");
    const entryJs = readFileSync(`dist/bundle/${entryBase}.js`, "utf-8");
    const bundledJs = runtimeJs + "\n" + entryJs;

    // Generate C header
    console.log("Generating host bundle...");
    const escapedJs = JSON.stringify(bundledJs);
    const headerContent = `#ifndef METASCRIPT_BUNDLE_H\n#define METASCRIPT_BUNDLE_H\nstatic const char* METASCRIPT_BUNDLE = ${escapedJs};\n#endif\n`;

    if (!existsSync("dist")) mkdirSync("dist");
    writeFileSync("dist/bundle.h", headerContent);

    // Compile C++ host
    console.log("Compiling binary...");
    const cflags = spawnSync("pkg-config", ["--cflags", "gtk+-3.0", "webkit2gtk-4.1"]).stdout.toString().trim().split(" ");
    const libs = spawnSync("pkg-config", ["--libs", "gtk+-3.0", "webkit2gtk-4.1"]).stdout.toString().trim().split(" ");

    const compileArgs = [
        "-std=c++11",
        "-Icore/include",
        "-Idist",
        "src/host.cpp",
        "core/src/webview.cc",
        "-o", output,
        "-lutil", // for forkpty
        ...cflags,
        ...libs
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
