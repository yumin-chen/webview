import { ControlProps } from "../components";
export interface RadioButtonProps extends ControlProps { selected: boolean; label?: string; name: string; value: string; }
export function RadioButton(props: RadioButtonProps): any { return { type: "RadioButton", props }; }
