import { expect, test, describe } from "bun:test";
import { Alloy } from "../src/index";

// Mocking window.Alloy for tests
if (typeof window === "undefined") {
    (global as any).window = {};
}
(window as any).Alloy = {
    transpiler_transform: async (code: string) => `transpiled: ${code}`,
    transpiler_transform_sync: (code: string) => `transpiled: ${code}`,
    transpiler_scan: (code: string) => JSON.stringify({ exports: ["App"], imports: [{ path: "react", kind: "import-statement" }] })
};

describe("Alloy Transpiler API", () => {
  test("transform method", async () => {
    const transpiler = new Alloy.Transpiler({ loader: "tsx" });
    const result = await transpiler.transform("const x: number = 1;");
    expect(result).toContain("transpiled");
  });

  test("transformSync method", () => {
    const transpiler = new Alloy.Transpiler();
    const result = transpiler.transformSync("<div></div>", "jsx");
    expect(result).toContain("transpiled");
  });

  test("browser target", async () => {
    const transpiler = new Alloy.Transpiler({ target: "browser" });
    const result = await transpiler.transform("const x = 1;");
    // Should mock WASM output logic
    expect(result).toBeDefined();
  });

  test("scan and scanImports", () => {
    const transpiler = new Alloy.Transpiler();
    const result = transpiler.scan("import React from 'react'");
    expect(result.exports).toContain("App");
    expect(result.imports[0].path).toBe("react");

    const imports = transpiler.scanImports("...");
    expect(imports.length).toBe(1);
  });
});
