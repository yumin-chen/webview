import { expect, test, describe } from "bun:test";

describe("Alloy.gui E2E", () => {
    test("should create a button and handle text", async () => {
        const win = Alloy.gui.createWindow("E2E Test", 200, 200);
        const btn = Alloy.gui.createButton(win);
        btn.setText("Initial");
        expect(btn.handle).toBeDefined();
    });

    test("should handle reactivity with signals", () => {
        const titleSig = new Alloy.gui.Signal("App Title");
        const win = Alloy.gui.createWindow("React Test", 300, 300);
        win.bind(Alloy.gui.Props.TEXT, titleSig);

        const btn = Alloy.gui.createButton(win);
        btn.bind(Alloy.gui.Props.TEXT, titleSig);

        titleSig.set("Updated Title");
    });

    test("should support all component creation methods", () => {
        const win = Alloy.gui.createWindow("Gallery", 800, 600);
        const vstack = Alloy.gui.createVStack(win);

        const components = [
            'Button', 'TextField', 'TextArea', 'Label', 'CheckBox', 'RadioButton',
            'ComboBox', 'Slider', 'Spinner', 'ProgressBar', 'TabView', 'ListView',
            'TreeView', 'WebView', 'HStack', 'ScrollView', 'Switch', 'Separator',
            'Image', 'Icon', 'MenuBar', 'Toolbar', 'StatusBar', 'Splitter',
            'Dialog', 'FileDialog', 'ColorPicker', 'DatePicker', 'TimePicker',
            'Link', 'Chip', 'Accordion', 'CodeEditor', 'Tooltip', 'GroupBox',
            'Popover', 'Badge', 'Card', 'Rating', 'Menu', 'ContextMenu', 'Divider',
            'LoadingIndicator', 'RichTextEditor'
        ];

        for (const type of components) {
            const createFn = Alloy.gui[`create${type}`];
            expect(createFn).toBeDefined();
            // Some take title/width/height, others take parent
            let comp;
            if (type === 'Dialog') {
                comp = createFn("Dialog", 400, 300);
            } else {
                comp = createFn(vstack);
            }
            expect(comp.handle).toBeDefined();
        }
    });
});
