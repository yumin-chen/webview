import { expect, test, describe, spyOn } from "bun:test";
import { Button, createComponent } from "../../src/gui";

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

describe("Alloy:gui > Button", () => {
  test("creation with props", () => {
    const btn = Button({ label: "Click", variant: "primary", enabled: false });
    expect(btn.type).toBe("Button");
    expect(btn.props.label).toBe("Click");
    expect(btn.props.variant).toBe("primary");
    expect(btn.props.enabled).toBe(false);
    const id = createComponent(btn.type, btn.props);
    expect(id).toBe(1);
  });
});
