import { ControlProps } from "../components";
export interface ButtonProps extends ControlProps { label: string; onClick?: () => void; }
export function Button(props: ButtonProps): any { return { type: "Button", props }; }
