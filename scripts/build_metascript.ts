import { build } from "bun";
import { readFileSync, writeFileSync } from "fs";
import { join } from "path";

async function buildMetaScript(entryPath: string, outputPath: string) {
  // 1. Transpile MetaScript source to JS using Bun.build
  const result = await build({
    entrypoints: [entryPath],
    minify: true,
    target: "browser", // Targeting webview
  });

  if (!result.success) {
    console.error("Build failed:", result.logs);
    process.exit(1);
  }

  const transpiledJS = await result.outputs[0].text();

  // 2. Embed transpiled JS into a C host program
  // We'll generate a C file that contains the JS as a static array
  const hexEncodedJS = Array.from(Buffer.from(transpiledJS))
    .map((b) => `0x${b.toString(16).padStart(2, "0")}`)
    .join(", ");

  const cSource = `
#include "metascript/runtime.hh"
#include <iostream>

static const unsigned char METASCRIPT_JS[] = { ${hexEncodedJS}, 0x00 };

int main(int argc, char** argv) {
    int res = metascript::runtime::handle_cli(argc, argv);
    if (res != -1) {
        return res;
    }

    try {
        metascript::runtime rt(false);
        rt.set_title("MetaScript Application");
        rt.set_size(800, 600, WEBVIEW_HINT_NONE);
        rt.set_html("<!DOCTYPE html><html><body><script>" + std::string((const char*)METASCRIPT_JS) + "</script></body></html>");
        rt.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
`;

  const cFilePath = outputPath + ".cc";
  writeFileSync(cFilePath, cSource);
  console.log(`Generated C++ source: ${cFilePath}`);
  console.log(`Now you can compile it with: g++ -Icore/include ${cFilePath} -o ${outputPath} ...`);
}

const args = process.argv.slice(2);
if (args.length < 2) {
  console.log("Usage: bun scripts/build_metascript.ts <entry.ts> <output_binary_name>");
  process.exit(1);
}

buildMetaScript(args[0], args[1]);
