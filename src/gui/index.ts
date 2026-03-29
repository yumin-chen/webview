import * as Components from "./components";
import { Color } from "./types";

declare global {
  interface Window {
    Alloy: {
      gui: {
        create: (type: string, props: any) => string; // returns component_handle (address)
        update: (handle: string, props: any) => void;
        destroy: (handle: string) => void;
      };
    };
  }
}

export const {
    Window, Button, TextField, TextArea, Label, CheckBox, RadioButton, ComboBox,
    Slider, Spinner, DatePicker, TimePicker, ColorPicker, Switch,
    Image, Icon, ProgressBar, Tooltip, Badge, Card, Divider, RichTextEditor,
    ListView, TreeView, TabView,
    VStack, HStack, ScrollView, GroupBox,
    Menu, MenuBar, Toolbar, ContextMenu,
    Dialog, FileDialog, Popover, StatusBar, Splitter,
    WebView, Link, Chip, Rating, Accordion, CodeEditor
} = Components;

export { Color };

export const createComponent = (type: string, props: any) => {
    return window.Alloy.gui.create(type, props);
};

export const updateComponent = (handle: string, props: any) => {
    window.Alloy.gui.update(handle, props);
};

export const destroyComponent = (handle: string) => {
    window.Alloy.gui.destroy(handle);
};
