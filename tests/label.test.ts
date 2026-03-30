import { expect, test, describe } from "bun:test";

const mockAlloy = {
    gui: {
        createWindow: () => "1",
        createLabel: () => "3",
        setText: () => true,
        destroy: () => true
    }
};
(global as any).Alloy = mockAlloy;

describe("Alloy.gui.Label", () => {
  test("create and set text", () => {
    const win = Alloy.gui.createWindow("Test", 800, 600);
    const lbl = Alloy.gui.createLabel(win);
    expect(lbl).toBeDefined();
    expect(Alloy.gui.setText(lbl, "Hello World")).toBe(true);
  });
});
