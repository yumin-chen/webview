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
export function VStack(props: StackProps): any { return { type: "VStack", props }; }
export function HStack(props: StackProps): any { return { type: "HStack", props }; }
