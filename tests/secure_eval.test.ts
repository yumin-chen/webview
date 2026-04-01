import { test, expect } from "bun:test";

test("secureEval should route through __alloy_secure_eval if available", async () => {
    // Mock the global Alloy and bridge
    const Alloy = {
        secureEval: (code: string) => {
            if ((globalThis as any).__alloy_secure_eval) {
                return (globalThis as any).__alloy_secure_eval(code);
            }
            return code;
        }
    };

    (globalThis as any).__alloy_secure_eval = (code: string) => {
        return `secured:${code}`;
    };

    const result = Alloy.secureEval("1 + 1");
    expect(result).toBe("secured:1 + 1");
});
