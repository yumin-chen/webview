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
import { ColorString, Padding, Margin, Border, BoxShadow } from "./types";

export interface ComponentStyle {
    // Spacing
    padding?: Padding;
    margin?: Margin;

    // Box
    width?: number | "fill";
    height?: number | "fill";
    backgroundColor?: ColorString;
    border?: Border;
    boxShadow?: BoxShadow;
    opacity?: number;

    // Text
    fontSize?: number;
    fontWeight?: "normal" | "bold" | "light" | number;
    fontStyle?: "normal" | "italic";
    fontFamily?: string;
    color?: ColorString;
    textAlign?: "left" | "center" | "right";
    lineHeight?: number;
    letterSpacing?: number;
}

export interface LayoutProps {
    // Sizing
    width?: number | "fill";
    height?: number | "fill";
    minWidth?: number;
    minHeight?: number;
    maxWidth?: number;
    maxHeight?: number;

    // Flex
    flex?: number;
    flexGrow?: number;
    flexShrink?: number;
    flexBasis?: number | "auto";

    // Alignment
    justifyContent?:
        | "start" | "end" | "center"
        | "spaceBetween" | "spaceAround" | "spaceEvenly";
    alignItems?: "start" | "end" | "center" | "stretch" | "baseline";
    alignContent?: "start" | "end" | "center" | "stretch" | "spaceBetween" | "spaceAround";

    // Direction
    flexDirection?: "row" | "column";
    flexWrap?: "no-wrap" | "wrap" | "wrap-reverse";

    // Gap
    gap?: number;
    rowGap?: number;
    columnGap?: number;
}
