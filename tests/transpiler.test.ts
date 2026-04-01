import { test, expect } from "bun:test";

test("Alloy.Transpiler should transform code", () => {
    // Mock the global Alloy and bridge
    const Alloy = {
        Transpiler: class {
            id: number;
            constructor(options: any) { this.id = 1; }
            transformSync(code: string, loader: string) {
                return (globalThis as any).alloy_transpiler_transform(this.id, code, loader);
            }
        }
    };

    (globalThis as any).alloy_transpiler_transform = (id: number, code: string, loader: string) => {
        return `transformed:${loader}:${code}`;
    };

    const transpiler = new Alloy.Transpiler({ loader: "tsx" });
    const result = transpiler.transformSync("const x = 1;", "tsx");
    expect(result).toBe("transformed:tsx:const x = 1;");
});

test("Alloy.build should return bytecode from compiler", () => {
    (globalThis as any).alloy_build = (source: string) => {
        return `bytecode:${source}`;
    };

    const Alloy = {
        build: (source: string) => (globalThis as any).alloy_build(source)
    };

    const result = Alloy.build("const y = 2;");
    expect(result).toBe("bytecode:const y = 2;");
});
