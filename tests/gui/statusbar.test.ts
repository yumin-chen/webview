import { expect, test, describe } from "bun:test";
import { StatusBar, createComponent } from "../../src/gui";

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

describe("Alloy:gui > StatusBar", () => {
  test("creation with props", () => {
    const element = StatusBar({ text: 's' });
    expect(element.type).toBe("StatusBar");
    const id = createComponent(element.type, element.props);
    expect(id).toBe(1);
  });
});
