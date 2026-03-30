import { expect, test, describe, spyOn } from "bun:test";
import { Window, createComponent } from "../../src/gui";

// Mocking window.Alloy for tests
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

describe("Alloy:gui > Window", () => {
  test("creation with complex props", () => {
    const win = Window({ title: "Test", width: 800, height: 600, resizable: true });
    expect(win.type).toBe("Window");
    expect(win.props.title).toBe("Test");
    expect(win.props.width).toBe(800);
    expect(win.props.resizable).toBe(true);
    const id = createComponent(win.type, win.props);
    expect(id).toBe(1);
  });
});
