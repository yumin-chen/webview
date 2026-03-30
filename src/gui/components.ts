import { ColorString, Padding } from "./types";
import { MouseEvent, KeyEvent, ResizeEvent, MoveEvent, DropEvent, WindowState } from "./events";
import { ComponentStyle, LayoutProps } from "./styling";

export type ReactNode = any;

export interface ComponentProps {
    id?: string;
    className?: string;
    style?: ComponentStyle & LayoutProps;
    key?: string;
}

export interface ControlProps extends ComponentProps {
    enabled?: boolean;
    focused?: boolean;
    tabIndex?: number;
}

export type { ColorString };

export * from "./components/Window";
export * from "./components/Button";
export * from "./components/TextField";
export * from "./components/TextArea";
export * from "./components/CheckBox";
export * from "./components/RadioButton";
export * from "./components/ComboBox";
export * from "./components/Slider";
export * from "./components/Spinner";
export * from "./components/DatePicker";
export * from "./components/TimePicker";
export * from "./components/ColorPicker";
export * from "./components/Switch";
export * from "./components/Label";
export * from "./components/Image";
export * from "./components/Icon";
export * from "./components/ProgressBar";
export * from "./components/Tooltip";
export * from "./components/Badge";
export * from "./components/Card";
export * from "./components/Divider";
export * from "./components/RichTextEditor";
export * from "./components/ListView";
export * from "./components/TreeView";
export * from "./components/TabView";
export * from "./components/VStack";
export * from "./components/HStack";
export * from "./components/ScrollView";
export * from "./components/GroupBox";
export * from "./components/Menu";
export * from "./components/MenuBar";
export * from "./components/Toolbar";
export * from "./components/ContextMenu";
export * from "./components/Dialog";
export * from "./components/FileDialog";
export * from "./components/Popover";
export * from "./components/StatusBar";
export * from "./components/Splitter";
export * from "./components/WebView";
export * from "./components/Link";
export * from "./components/Chip";
export * from "./components/Rating";
export * from "./components/Accordion";
export * from "./components/CodeEditor";
