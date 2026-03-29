export type ColorString = string & { readonly brand: "Color" };

export interface Spacing {
    top?: number;
    right?: number;
    bottom?: number;
    left?: number;
}

export type Padding = number | Spacing;
export type Margin = number | Spacing;

export interface Border {
    width?: number;
    color?: ColorString;
    style?: "solid" | "dashed" | "dotted";
    radius?: number;
}

export interface BoxShadow {
    offsetX?: number;
    offsetY?: number;
    blurRadius?: number;
    color?: ColorString;
}

export class Color {
    static black(): ColorString { return "#000000" as ColorString; }
    static white(): ColorString { return "#ffffff" as ColorString; }
    static gray(shade: number): ColorString { return `gray-${shade}` as ColorString; }
    static red(shade: number): ColorString { return `red-${shade}` as ColorString; }
    static blue(shade: number): ColorString { return `blue-${shade}` as ColorString; }
    static rgb(r: number, g: number, b: number): ColorString { return `rgb(${r},${g},${b})` as ColorString; }
    static hex(h: string): ColorString { return h as ColorString; }
}
