/*
 * AlloyScript Production Runtime
 *
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * For more information, please refer to <http://unlicense.org/>
 */
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
