import { expect, test, describe } from "bun:test";
import { Badge, createComponent } from "../../src/gui";

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

describe("Alloy:gui > Badge", () => {
  test("creation with props", () => {
    const element = Badge({ text: '1' });
    expect(element.type).toBe("Badge");
    const id = createComponent(element.type, element.props);
    expect(id).toBe(1);
  });
});
