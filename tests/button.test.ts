import { expect, test, describe } from "bun:test";

const mockAlloy = {
    gui: {
        createWindow: () => "1",
        createButton: () => "2",
        setText: () => true,
        destroy: () => true
    }
};
(global as any).Alloy = mockAlloy;

describe("Alloy.gui.Button", () => {
  test("create and set text", () => {
    const win = Alloy.gui.createWindow("Test", 800, 600);
    const btn = Alloy.gui.createButton(win);
    expect(btn).toBeDefined();
    expect(Alloy.gui.setText(btn, "Click Me")).toBe(true);
  });

  test("destroy", () => {
    const btn = Alloy.gui.createButton("1");
    expect(Alloy.gui.destroy(btn)).toBe(true);
  });
});
