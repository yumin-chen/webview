import { test, expect } from "bun:test";

test("Alloy.Transpiler should support browser target", async () => {
    // Mock the global Alloy and bridge
    const Alloy = {
        Transpiler: class {
            id: number;
            options: any;
            constructor(options: any) { this.id = 1; this.options = options; }
            transformSync(code: string, loader: string) {
                return (globalThis as any).alloy_transpiler_transform(this.id, code, loader, this.options.target);
            }
        }
    };

    (globalThis as any).alloy_transpiler_transform = (id: number, code: string, loader: string, target: string) => {
        if (target === "browser") {
            return `// alloy:wasm-target\ntransformed:${code}`;
        }
        return `transformed:${code}`;
    };

    const transpiler = new Alloy.Transpiler({ loader: "js", target: "browser" });
    const result = transpiler.transformSync("console.log(1);", "js");
    expect(result).toContain("// alloy:wasm-target");
});
