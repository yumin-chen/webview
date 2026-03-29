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

export interface WindowProps extends ComponentProps {
    title?: string;
    width?: number;
    height?: number;
    minWidth?: number;
    minHeight?: number;
    maxWidth?: number;
    maxHeight?: number;
    resizable?: boolean;
    minimizable?: boolean;
    maximizable?: boolean;
    closable?: boolean;
    fullscreen?: boolean;
    alwaysOnTop?: boolean;
    theme?: "light" | "dark" | "auto";
    opacity?: number;
    backgroundColor?: ColorString;
    onCreated?: () => void;
    onDestroy?: () => void;
    onFocus?: () => void;
    onBlur?: () => void;
    onResizeWindow?: (event: ResizeEvent) => void;
    onMoveWindow?: (event: MoveEvent) => void;
    onMinimize?: () => void;
    onMaximize?: () => void;
    onRestore?: () => void;
    onClose?: () => void | boolean;
    onStateChange?: (state: WindowState) => void;
    onDragEnter?: (event: DropEvent) => void;
    onDragOver?: (event: DropEvent) => void;
    onDrop?: (event: DropEvent) => void;
    children: ReactNode;
}

export interface ButtonProps extends ControlProps {
    label: string;
    icon?: string;
    variant?: "default" | "primary" | "secondary" | "danger";
    size?: "small" | "medium" | "large";
    width?: number | "fill";
    height?: number;
    onClick?: () => void;
    onDoubleClick?: () => void;
    onMouseEnter?: () => void;
    onMouseLeave?: () => void;
    onMouseDown?: (event: MouseEvent) => void;
    onMouseUp?: (event: MouseEvent) => void;
    onFocus?: () => void;
    onBlur?: () => void;
    onKeyDown?: (event: KeyEvent) => void;
    onKeyUp?: (event: KeyEvent) => void;
}

export interface TextFieldProps extends ControlProps {
    value?: string;
    placeholder?: string;
    readonly?: boolean;
    maxLength?: number;
    pattern?: RegExp;
    width?: number | "fill";
    height?: number;
    onChange?: (value: string) => void;
    onFocus?: () => void;
    onBlur?: () => void;
    onKeyDown?: (event: KeyEvent) => void;
    onKeyUp?: (event: KeyEvent) => void;
    onEnter?: (value: string) => void;
}

export interface TextAreaProps extends ControlProps {
    value?: string;
    placeholder?: string;
    readonly?: boolean;
    maxLength?: number;
    width?: number | "fill";
    height?: number | "fill";
    scrollable?: boolean;
    wordWrap?: boolean;
}

export interface LabelProps extends ComponentProps {
    text: string;
    width?: number | "fill";
    height?: number;
}

export interface CheckBoxProps extends ControlProps {
    checked: boolean;
    label?: string;
}

export interface RadioButtonProps extends ControlProps {
    name: string;
    value: string;
    selected?: boolean;
    label?: string;
}

export interface ComboBoxOption {
    label: string;
    value: string;
}

export interface ComboBoxProps extends ControlProps {
    options: ComboBoxOption[];
    selectedValue?: string;
}

export interface SliderProps extends ControlProps {
    value: number;
    min?: number;
    max?: number;
}

export interface SpinnerProps extends ControlProps {
    value: number;
    min?: number;
    max?: number;
}

export interface SwitchProps extends ControlProps {
    checked: boolean;
}

export interface ProgressBarProps extends ComponentProps {
    value?: number;
    indeterminate?: boolean;
}

export interface ListViewProps extends ControlProps {
    items: any[];
}

export interface TreeViewProps extends ControlProps {
    root: any;
}

export interface TabViewProps extends ControlProps {
    tabs: any[];
}

export interface WebViewProps extends ControlProps {
    src?: string;
    html?: string;
}

export interface ScrollViewProps extends ComponentProps {
    horizontal?: boolean;
    vertical?: boolean;
    children: ReactNode;
}

// Containers
export interface StackProps extends ComponentProps {
    spacing?: number;
    padding?: Padding;
    alignItems?: "start" | "center" | "end" | "stretch";
    justifyContent?: "start" | "center" | "end" | "spaceBetween" | "spaceAround";
    width?: number | "fill";
    height?: number | "fill";
    backgroundColor?: ColorString;
    borderRadius?: number;
    border?: any;
    onClick?: () => void;
    children: ReactNode[];
}

export function Window(props: WindowProps): any { return { type: "Window", props }; }
export function Button(props: ButtonProps): any { return { type: "Button", props }; }
export function TextField(props: TextFieldProps): any { return { type: "TextField", props }; }
export function TextArea(props: TextAreaProps): any { return { type: "TextArea", props }; }
export function Label(props: LabelProps): any { return { type: "Label", props }; }
export function CheckBox(props: CheckBoxProps): any { return { type: "CheckBox", props }; }
export function RadioButton(props: RadioButtonProps): any { return { type: "RadioButton", props }; }
export function ComboBox(props: ComboBoxProps): any { return { type: "ComboBox", props }; }
export function Slider(props: SliderProps): any { return { type: "Slider", props }; }
export function Spinner(props: SpinnerProps): any { return { type: "Spinner", props }; }
export function Switch(props: SwitchProps): any { return { type: "Switch", props }; }
export function ProgressBar(props: ProgressBarProps): any { return { type: "ProgressBar", props }; }
export function ListView(props: ListViewProps): any { return { type: "ListView", props }; }
export function TreeView(props: TreeViewProps): any { return { type: "TreeView", props }; }
export function TabView(props: TabViewProps): any { return { type: "TabView", props }; }
export function WebView(props: WebViewProps): any { return { type: "WebView", props }; }
export function VStack(props: StackProps): any { return { type: "VStack", props }; }
export function HStack(props: StackProps): any { return { type: "HStack", props }; }
export function ScrollView(props: ScrollViewProps): any { return { type: "ScrollView", props }; }
