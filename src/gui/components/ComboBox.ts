import { ControlProps } from "../components";
export interface ComboBoxProps extends ControlProps { options: {label: string, value: string}[]; selectedValue?: string; }
export function ComboBox(props: ComboBoxProps): any { return { type: "ComboBox", props }; }
