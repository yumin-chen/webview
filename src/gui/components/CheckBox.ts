import { ControlProps } from "../components";
export interface CheckBoxProps extends ControlProps { checked: boolean; label?: string; onChange?: (c: boolean) => void; }
export function CheckBox(props: CheckBoxProps): any { return { type: "CheckBox", props }; }
