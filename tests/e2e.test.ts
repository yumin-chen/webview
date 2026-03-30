import { expect, test, describe, spyOn } from "bun:test";
import {
    createComponent, updateComponent, addChild,
    Window, Button, TextField, VStack
} from "../src/gui";

// Mocking window.Alloy for E2E tests
if (typeof window === "undefined") {
    (global as any).window = {};
}
(window as any).Alloy = (window as any).Alloy || {};
(window as any).Alloy.gui = {
    create: (type: string, props: any) => 1,
    update: (id: number, props: any) => {},
    destroy: (id: number) => {},
    addChild: (parent: number, child: number) => {}
};

describe("Alloy:gui E2E Bridge", () => {
    test("Full lifecycle: tree construction and updates", () => {
        const createSpy = spyOn((window as any).Alloy.gui, "create").mockImplementation((type: string, props: any) => {
            if (type === "Window") return 100;
            if (type === "VStack") return 101;
            if (type === "Button") return 102;
            return 1;
        });
        const updateSpy = spyOn((window as any).Alloy.gui, "update");
        const addChildSpy = spyOn((window as any).Alloy.gui, "addChild");

        // 1. Create Window
        const win = Window({ title: "App", width: 500 });
        const winId = createComponent(win.type, win.props);
        expect(winId).toBe(100);
        expect(createSpy).toHaveBeenCalledWith("Window", win.props);

        // 2. Create Layout
        const layout = VStack({ spacing: 10, children: [] });
        const layoutId = createComponent(layout.type, layout.props);
        expect(layoutId).toBe(101);
        addChild(winId, layoutId);
        expect(addChildSpy).toHaveBeenCalledWith(winId, layoutId);

        // 3. Create Button inside layout
        const btn = Button({ label: "OK" });
        const btnId = createComponent(btn.type, btn.props);
        expect(btnId).toBe(102);
        addChild(layoutId, btnId);
        expect(addChildSpy).toHaveBeenCalledWith(layoutId, btnId);

        // 4. Update property
        updateComponent(btnId, { label: "Done" });
        expect(updateSpy).toHaveBeenCalledWith(btnId, { label: "Done" });
    });
});
