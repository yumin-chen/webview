import { ControlProps } from "../components";
export interface SwitchProps extends ControlProps { checked: boolean; }
export function Switch(props: SwitchProps): any { return { type: "Switch", props }; }
