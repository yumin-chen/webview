/**
 * @alloyscript/runtime GUI types
 */

export type Color = string & { readonly brand: "Color" };
export type Font = string & { readonly brand: "Font" };

export interface Spacing {
    top?: number;
    bottom?: number;
    left?: number;
    right?: number;
}

export type Padding = number | Spacing;
export type Margin = number | Spacing;

export interface Border {
    width?: number;
    color?: Color;
    style?: "solid" | "dashed" | "dotted";
    radius?: number;
}

export interface BoxShadow {
    offsetX?: number;
    offsetY?: number;
    blurRadius?: number;
    color?: Color;
}

export interface ComponentStyle {
    padding?: Padding;
    margin?: Margin;
    width?: number | "fill";
    height?: number | "fill";
    backgroundColor?: Color;
    border?: Border;
    boxShadow?: BoxShadow;
    opacity?: number;
    fontSize?: number;
    fontWeight?: "normal" | "bold" | "light" | number;
    fontStyle?: "normal" | "italic";
    fontFamily?: string;
    color?: Color;
    textAlign?: "left" | "center" | "right";
    lineHeight?: number;
    letterSpacing?: number;
}

export interface ComponentProps {
    id?: string;
    className?: string;
    style?: ComponentStyle;
    key?: string;
}

export interface ControlProps extends ComponentProps {
    enabled?: boolean;
    focused?: boolean;
    tabIndex?: number;
}

export interface WindowProps {
    title?: string;
    width?: number;
    height?: number;
    minWidth?: number;
    minHeight?: number;
    maxWidth?: number;
    maxHeight?: number;
    resizable?: boolean;
    closable?: boolean;
    children?: any;
}

export function Window(props: WindowProps): any { return { type: "Window", props }; }
export function Button(props: any): any { return { type: "Button", props }; }
export function TextField(props: any): any { return { type: "TextField", props }; }
export function TextArea(props: any): any { return { type: "TextArea", props }; }
export function Label(props: any): any { return { type: "Label", props }; }
export function VStack(props: any): any { return { type: "VStack", props }; }
export function HStack(props: any): any { return { type: "HStack", props }; }

export const Color = {
    rgb: (r: number, g: number, b: number) => `rgb(${r},${g},${b})` as Color,
    hex: (h: string) => h as Color,
    white: () => "#ffffff" as Color,
    black: () => "#000000" as Color,
    blue: (v: number) => `blue-${v}` as Color,
};
