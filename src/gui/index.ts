import { Window, Button, TextField, TextArea, Label, CheckBox, RadioButton, ComboBox, Slider, Spinner, Switch, ProgressBar, ListView, TreeView, TabView, WebView, VStack, HStack, ScrollView, Menu, MenuBar, Toolbar, StatusBar, Splitter, Dialog, FileDialog, ColorPicker, DatePicker, TimePicker, Tooltip, Divider, Image, Icon, Separator, GroupBox, Accordion, Popover, ContextMenu, Badge, Chip, SpinnerLoading, Card, Link, Rating, RichText, CodeEditor } from "./components";
import { Color } from "./types";

declare global {
  interface Window {
    Alloy: {
      gui: {
        create: (type: string, props: any) => number; // returns component_id
        update: (id: number, props: any) => void;
        destroy: (id: number) => void;
        addChild: (parent: number, child: number) => void;
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
    Menu,
    MenuBar,
    Toolbar,
    StatusBar,
    Splitter,
    Dialog,
    FileDialog,
    ColorPicker,
    DatePicker,
    TimePicker,
    Tooltip,
    Divider,
    Image,
    Icon,
    Separator,
    GroupBox,
    Accordion,
    Popover,
    ContextMenu,
    Badge,
    Chip,
    SpinnerLoading,
    Card,
    Link,
    Rating,
    RichText,
    CodeEditor,
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

export const addChild = (parent: number, child: number) => {
    window.Alloy.gui.addChild(parent, child);
};
