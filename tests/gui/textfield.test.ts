import { expect, test, describe, spyOn } from "bun:test";
import { TextField, createComponent } from "../../src/gui";

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

describe("Alloy:gui > TextField", () => {
  test("creation with props", () => {
    const field = TextField({ value: "hello", placeholder: "type...", maxLength: 10 });
    expect(field.type).toBe("TextField");
    expect(field.props.value).toBe("hello");
    expect(field.props.placeholder).toBe("type...");
    expect(field.props.maxLength).toBe(10);
    const id = createComponent(field.type, field.props);
    expect(id).toBe(1);
  });
});
