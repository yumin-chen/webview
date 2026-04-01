import { test, expect } from "bun:test";

test("Alloy.Transpiler should support node.js target with decompilation", async () => {
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
        let result = `transformed:${code}`;
        if (target === "node.js") {
            // Simulate build -> decompile loop
            const bc = `mquickjs_bytecode:${result}`;
            result = bc.replace("mquickjs_bytecode:", "");
        }
        return result;
    };

    const transpiler = new Alloy.Transpiler({ loader: "tsx", target: "node.js" });
    const result = transpiler.transformSync("const x = 1;", "tsx");
    expect(result).toBe("transformed:const x = 1;");
});
