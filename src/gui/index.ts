import { Window, Button, TextField, TextArea, Label, CheckBox, RadioButton, ComboBox, Slider, Spinner, Switch, ProgressBar, ListView, TreeView, TabView, WebView, VStack, HStack, ScrollView } from "./components";
import { Color } from "./types";

declare global {
  interface Window {
    Alloy: {
      gui: {
        create: (type: string, props: any) => number; // returns component_id
        update: (id: number, props: any) => void;
        destroy: (id: number) => void;
      };
    };
  }
}

export {
    Window,
    Button,
    TextField,
    TextArea,
    Label,
    CheckBox,
    RadioButton,
    ComboBox,
    Slider,
    Spinner,
    Switch,
    ProgressBar,
    ListView,
    TreeView,
    TabView,
    WebView,
    VStack,
    HStack,
    ScrollView,
    Color
};

export const createComponent = (type: string, props: any) => {
    return window.Alloy.gui.create(type, props);
};

export const updateComponent = (id: number, props: any) => {
    window.Alloy.gui.update(id, props);
};

export const destroyComponent = (id: number) => {
    window.Alloy.gui.destroy(id);
};
