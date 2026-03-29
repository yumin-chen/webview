import { expect, test, describe } from "bun:test";
import { createComponent, updateComponent, destroyComponent, Window, Button, Color } from "../src/gui";

// Mocking window.Alloy for tests
if (typeof window === "undefined") {
    (global as any).window = {};
}
(window as any).Alloy = (window as any).Alloy || {};
(window as any).Alloy.gui = {
    create: (type: string, props: any) => 1,
    update: (id: number, props: any) => {},
    destroy: (id: number) => {}
};

describe("Alloy:gui", () => {
  test("Component object creation (ASX representation)", () => {
    const btn = Button({ label: "Click Me", variant: "primary" });
    expect(btn.type).toBe("Button");
    expect(btn.props.label).toBe("Click Me");
  });

  test("Bridge createComponent", () => {
    const id = createComponent("Button", { label: "Test" });
    expect(id).toBe(1);
  });

  test("Styling and Color API", () => {
      const color = Color.blue(500);
      expect(color).toBe("blue-500");
  });

  test("Complex Layout props", () => {
      const win = Window({
          title: "My App",
          width: 800,
          children: [
              Button({ label: "OK" })
          ]
      });
      expect(win.props.title).toBe("My App");
      expect(win.props.children).toHaveLength(1);
  });
});
