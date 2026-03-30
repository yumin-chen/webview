globalThis.window = globalThis;
import { expect, test, describe, beforeAll } from "bun:test";

const setupMock = () => {
    (globalThis as any).Alloy = {
        gui: {
            create: (type: string, props: any) => ({ type, props }),
            createSignal: (initial: any) => ({ value: initial })
        }
    };
};

describe("UI Components", () => {
    beforeAll(setupMock);

    test("VStack should be creatable", async () => {
        const mod = await import("../src/gui");
        const component = (mod as any)["VStack"]({ title: "Test" });
        expect(component.type).toBe("VStack");
    });
});
