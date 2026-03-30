import { expect, test, describe } from "bun:test";

const mockAlloy = {
    gui: {
        createWindow: () => "1",
        createSlider: () => "7",
        destroy: () => true
    }
};
(global as any).Alloy = mockAlloy;

describe("Alloy.gui.Slider", () => {
  test("create", () => {
    const win = Alloy.gui.createWindow("Test", 800, 600);
    const slider = Alloy.gui.createSlider(win);
    expect(slider).toBeDefined();
  });
});
