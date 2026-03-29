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
