import { expect, test, describe } from "bun:test";

const mockAlloy = {
    gui: {
        createWindow: () => "1",
        createTextArea: () => "5",
        setText: () => true,
        destroy: () => true
    }
};
(global as any).Alloy = mockAlloy;

describe("Alloy.gui.TextArea", () => {
  test("create and set text", () => {
    const win = Alloy.gui.createWindow("Test", 800, 600);
    const txt = Alloy.gui.createTextArea(win);
    expect(txt).toBeDefined();
    expect(Alloy.gui.setText(txt, "Multi-line\ntext")).toBe(true);
  });
});
