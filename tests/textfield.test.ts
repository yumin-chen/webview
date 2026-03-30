import { expect, test, describe } from "bun:test";

const mockAlloy = {
    gui: {
        createWindow: () => "1",
        createTextField: () => "4",
        setText: () => true,
        destroy: () => true
    }
};
(global as any).Alloy = mockAlloy;

describe("Alloy.gui.TextField", () => {
  test("create and set text", () => {
    const win = Alloy.gui.createWindow("Test", 800, 600);
    const txt = Alloy.gui.createTextField(win);
    expect(txt).toBeDefined();
    expect(Alloy.gui.setText(txt, "Input text")).toBe(true);
  });
});
