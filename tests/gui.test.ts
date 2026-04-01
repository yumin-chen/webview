import { expect, test, describe } from "bun:test";

describe("Alloy.gui", () => {
    test("should create window and components", () => {
        const win = Alloy.gui.createWindow("Test", 100, 100);
        expect(win).toBeDefined();
        const btn = Alloy.gui.createButton(win);
        expect(btn).toBeDefined();
    });

    test("should support signals and binding", () => {
        const sig = new Alloy.gui.Signal("hello");
        const win = Alloy.gui.createWindow("Test", 100, 100);
        win.bind(Alloy.gui.Props.TEXT, sig);
        sig.set("world");
    });
});
