import { ControlProps, ColorString } from "../components";
export interface ColorPickerProps extends ControlProps { color?: ColorString; }
export function ColorPicker(props: ColorPickerProps): any { return { type: "ColorPicker", props }; }
