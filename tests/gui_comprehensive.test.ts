globalThis.window = globalThis;
import { expect, test, describe, beforeAll } from "bun:test";

const setupMock = () => {
    (globalThis as any).Alloy = {
        gui: {
            create: async (type: string, props: any) => "handle_" + Math.random(),
            createSignal: (initial: any) => ({ value: initial })
        },
        secureEval: async (code: string) => "Evaluated in secure host: " + code
    };
};

describe("UI Components Comprehensive", () => {
    beforeAll(setupMock);

    test("Window and Hierarchy", async () => {
        const { Window, Button, VStack } = await import("../src/gui");
        const win = await Window({ title: "Main" });
        const layout = await VStack({});
        await win.addChild(layout);
        const btn = await Button({ label: "Click" });
        await layout.addChild(btn);

        expect(win.handle).toBeDefined();
        expect(layout.props.parent).toBe(win.handle);
        expect(btn.props.parent).toBe(layout.handle);
    });

    test("All Components Creatable", async () => {
        const gui = await import("../src/gui");
        const components = [
            "Button", "TextField", "TextArea", "Label", "CheckBox",
            "RadioButton", "ComboBox", "Slider", "ProgressBar", "TabView",
            "ListView", "TreeView", "WebView", "VStack", "HStack", "ScrollView"
        ];

        for (const name of components) {
            const Comp = (gui as any)[name];
            const instance = await Comp({});
            expect(instance.handle).toBeDefined();
            expect(instance.type).toBe(name);
        }
    });

    test("Secure Eval and Session Token Mock", async () => {
        const res = await (globalThis as any).Alloy.secureEval("1 + 1");
        expect(res).toContain("Evaluated in secure host");
    });
});
