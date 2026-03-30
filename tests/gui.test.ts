import { expect, test, describe, spyOn } from "bun:test";
import {
    createComponent, updateComponent, destroyComponent,
    Window, Button, TextField, Color, VStack, HStack,
    TextArea, Label, CheckBox, RadioButton, ComboBox,
    Slider, Spinner, Switch, ProgressBar, ListView,
    TreeView, TabView, WebView, ScrollView,
    Menu, MenuBar, Toolbar, StatusBar, Splitter,
    Dialog, FileDialog, ColorPicker, DatePicker,
    TimePicker, Tooltip, Divider, Image, Icon,
    Separator, GroupBox, Accordion, Popover,
    ContextMenu, Badge, Chip, SpinnerLoading,
    Card, Link, Rating, RichText, CodeEditor
} from "../src/gui";

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
            { fn: Spinner, props: { value: 10 }, type: "Spinner" },
            { fn: Switch, props: { checked: true }, type: "Switch" },
            { fn: ProgressBar, props: { value: 0.5 }, type: "ProgressBar" },
            { fn: ListView, props: { items: [] }, type: "ListView" },
            { fn: TreeView, props: { root: {} }, type: "TreeView" },
            { fn: TabView, props: { tabs: [] }, type: "TabView" },
            { fn: WebView, props: { src: "url" }, type: "WebView" },
            { fn: ScrollView, props: { children: [] }, type: "ScrollView" },
            { fn: Menu, props: { label: "m" }, type: "Menu" },
            { fn: MenuBar, props: { children: [] }, type: "MenuBar" },
            { fn: Toolbar, props: { children: [] }, type: "Toolbar" },
            { fn: StatusBar, props: { text: "s" }, type: "StatusBar" },
            { fn: Splitter, props: { orientation: "vertical" }, type: "Splitter" },
            { fn: Dialog, props: { title: "d" }, type: "Dialog" },
            { fn: FileDialog, props: { mode: "open" }, type: "FileDialog" },
            { fn: ColorPicker, props: { color: "#f00" }, type: "ColorPicker" },
            { fn: DatePicker, props: { date: "today" }, type: "DatePicker" },
            { fn: TimePicker, props: { time: "now" }, type: "TimePicker" },
            { fn: Tooltip, props: { text: "t" }, type: "Tooltip" },
            { fn: Divider, props: {}, type: "Divider" },
            { fn: Image, props: { src: "i" }, type: "Image" },
            { fn: Icon, props: { name: "i" }, type: "Icon" },
            { fn: Separator, props: {}, type: "Separator" },
            { fn: GroupBox, props: { label: "g", children: [] }, type: "GroupBox" },
            { fn: Accordion, props: { children: [] }, type: "Accordion" },
            { fn: Popover, props: { children: [] }, type: "Popover" },
            { fn: ContextMenu, props: { children: [] }, type: "ContextMenu" },
            { fn: Badge, props: { text: "1" }, type: "Badge" },
            { fn: Chip, props: { label: "c" }, type: "Chip" },
            { fn: SpinnerLoading, props: {}, type: "SpinnerLoading" },
            { fn: Card, props: { children: [] }, type: "Card" },
            { fn: Link, props: { text: "l", url: "u" }, type: "Link" },
            { fn: Rating, props: { value: 5 }, type: "Rating" },
            { fn: RichText, props: { html: "h" }, type: "RichText" },
            { fn: CodeEditor, props: { code: "c", language: "js" }, type: "CodeEditor" }
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
