import { ControlProps } from "../components";
export interface SpinnerProps extends ControlProps { value: number; step?: number; }
export function Spinner(props: SpinnerProps): any { return { type: "Spinner", props }; }
