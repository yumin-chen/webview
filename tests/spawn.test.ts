import { expect, test, describe, beforeAll } from "bun:test";
import { Alloy } from "../src/alloyscript/runtime";

describe("Alloy.spawn and Alloy.spawnSync", () => {
    // Mock AlloyInternal since we are running in Bun for tests
    beforeAll(() => {
        (globalThis as any).AlloyInternal = {
            spawnSync: (command: string, args: string[], options?: any) => {
                if (command === "echo") {
                    return {
                        stdout: new TextEncoder().encode(args.join(" ") + "\n"),
                        stderr: new Uint8Array(),
                        status: 0
                    };
                }
                return { stdout: new Uint8Array(), stderr: new Uint8Array(), status: 1 };
            },
            spawn: async (command: string, args: string[], options?: any) => {
                if (command === "echo") {
                    return {
                        stdout: new TextEncoder().encode(args.join(" ") + "\n"),
                        stderr: new Uint8Array(),
                        status: 0
                    };
                }
                return { stdout: new Uint8Array(), stderr: new Uint8Array(), status: 1 };
            }
        };
    });

    test("spawnSync should return correct result", () => {
        const result = Alloy.spawnSync("echo", ["hello", "world"]);
        expect(result.status).toBe(0);
        expect(new TextDecoder().decode(result.stdout)).toBe("hello world\n");
    });

    test("spawn should return correct result", async () => {
        const result = await Alloy.spawn("echo", ["hello", "world"]);
        expect(result.status).toBe(0);
        expect(new TextDecoder().decode(result.stdout)).toBe("hello world\n");
    });
});
