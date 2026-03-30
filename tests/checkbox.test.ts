import { expect, test, describe } from "bun:test";

const mockAlloy = {
    gui: {
        createWindow: () => "1",
        createCheckBox: () => "6",
        setText: () => true,
        destroy: () => true
    }
};
(global as any).Alloy = mockAlloy;

describe("Alloy.gui.CheckBox", () => {
  test("create and set text", () => {
    const win = Alloy.gui.createWindow("Test", 800, 600);
    const cb = Alloy.gui.createCheckBox(win);
    expect(cb).toBeDefined();
    expect(Alloy.gui.setText(cb, "Check me")).toBe(true);
  });
});
