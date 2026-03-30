import { expect, test, describe } from "bun:test";
import { Spinner, createComponent } from "../../src/gui";

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

describe("Alloy:gui > Spinner", () => {
  test("creation with props", () => {
    const element = Spinner({ value: 10 });
    expect(element.type).toBe("Spinner");
    const id = createComponent(element.type, element.props);
    expect(id).toBe(1);
  });
});
