import { ControlProps } from "../components";
export interface TextFieldProps extends ControlProps { value?: string; placeholder?: string; onChange?: (v: string) => void; }
export function TextField(props: TextFieldProps): any { return { type: "TextField", props }; }
