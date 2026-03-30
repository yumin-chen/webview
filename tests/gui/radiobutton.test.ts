import { expect, test, describe } from "bun:test";
import { RadioButton, createComponent } from "../../src/gui";

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

describe("Alloy:gui > RadioButton", () => {
  test("creation with props", () => {
    const element = RadioButton({ label: "radio", name: "g1", value: "v1" });
    expect(element.type).toBe("RadioButton");
    expect(element.props.label).toBe("radio");
    expect(element.props.name).toBe("g1");
    const id = createComponent(element.type, element.props);
    expect(id).toBe(1);
  });
});
