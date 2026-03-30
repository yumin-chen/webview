import { expect, test, describe } from "bun:test";
import { Slider, createComponent } from "../../src/gui";

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

describe("Alloy:gui > Slider", () => {
  test("creation with props", () => {
    const element = Slider({ value: 50, min: 0, max: 100 });
    expect(element.type).toBe("Slider");
    expect(element.props.value).toBe(50);
    const id = createComponent(element.type, element.props);
    expect(id).toBe(1);
  });
});
