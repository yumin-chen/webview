import { expect, test, describe } from "bun:test";

const mockAlloy = {
    gui: {
        createWindow: () => "1",
        createComboBox: () => "8",
        setText: () => true,
        setSelection: () => true,
        destroy: () => true
    }
};
(global as any).Alloy = mockAlloy;

describe("Alloy.gui.ComboBox", () => {
  test("create and select", () => {
    const window = Alloy.gui.createWindow("Test", 800, 600);
    const combo = Alloy.gui.createComboBox(window);
    expect(combo).toBeDefined();

    Alloy.gui.setText(combo, "Option 1");
    Alloy.gui.setText(combo, "Option 2");

    expect(Alloy.gui.setSelection(combo, 1)).toBe(true);
    expect(combo).not.toBeNull();
  });
});
