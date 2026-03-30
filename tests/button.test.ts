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

    test("Button should be creatable", async () => {
        const { Button } = await import("../src/gui");
        const btn = Button({ label: "Click Me" });
        expect(btn.type).toBe("Button");
        expect(btn.props.label).toBe("Click Me");
    });
});
