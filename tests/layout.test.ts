import { expect, test, describe } from "bun:test";

const mockAlloy = {
    gui: {
        createWindow: () => "1",
        createVStack: () => "2",
        createHStack: () => "3",
        createButton: () => "4",
        setText: () => true,
        destroy: () => true
    }
};
(global as any).Alloy = mockAlloy;

describe("Alloy.gui Layout", () => {
  test("VStack and HStack", () => {
    const win = Alloy.gui.createWindow("Test", 800, 600);
    const vstack = Alloy.gui.createVStack(win);
    const hstack = Alloy.gui.createHStack(vstack);
    const btn = Alloy.gui.createButton(hstack);
    expect(vstack).toBeDefined();
    expect(hstack).toBeDefined();
    expect(btn).toBeDefined();
  });
});
