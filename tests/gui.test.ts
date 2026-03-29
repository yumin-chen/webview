import { expect, test, describe, spyOn } from "bun:test";
import { createComponent, updateComponent, destroyComponent, Window, Button, TextField, Color, VStack, HStack, TextArea, Label, CheckBox, RadioButton, ComboBox, Slider, ProgressBar } from "../src/gui";

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

  describe("Native Component Generation", () => {
    test("TextField creation", () => {
      const field = TextField({ value: "hello", placeholder: "type..." });
      expect(field.type).toBe("TextField");
      expect(field.props.value).toBe("hello");
      const id = createComponent(field.type, field.props);
      expect(id).toBe(1);
    });

    test("VStack and HStack layout", () => {
      const layout = VStack({
        children: [
          HStack({ children: [Button({ label: "L" }), Button({ label: "R" })] }),
          TextField({ value: "bottom" })
        ]
      });
      expect(layout.type).toBe("VStack");
      expect(layout.props.children).toHaveLength(2);
      expect(layout.props.children[0].type).toBe("HStack");
    });

    test("Comprehensive component test suite", () => {
        const components = [
            { fn: TextArea, props: { value: "text" }, type: "TextArea" },
            { fn: Label, props: { text: "label" }, type: "Label" },
            { fn: CheckBox, props: { label: "check", checked: true }, type: "CheckBox" },
            { fn: RadioButton, props: { label: "radio", name: "g1" }, type: "RadioButton" },
            { fn: ComboBox, props: { options: [] }, type: "ComboBox" },
            { fn: Slider, props: { value: 50 }, type: "Slider" },
            { fn: ProgressBar, props: { value: 0.5 }, type: "ProgressBar" }
        ];

        components.forEach(comp => {
            const element = comp.fn(comp.props as any);
            expect(element.type).toBe(comp.type);
            const id = createComponent(element.type, element.props);
            expect(id).toBe(1);
        });
    });

    test("Component lifecycle: update and destroy", () => {
        const updateSpy = spyOn(window.Alloy.gui, "update");
        const destroySpy = spyOn(window.Alloy.gui, "destroy");

        updateComponent(1, { label: "New" });
        expect(updateSpy).toHaveBeenCalledWith(1, { label: "New" });

        destroyComponent(1);
        expect(destroySpy).toHaveBeenCalledWith(1);
    });
  });
});
