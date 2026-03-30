import { ControlProps } from "../components";
export interface ChipProps extends ControlProps { label: string; onDismiss?: () => void; }
export function Chip(props: ChipProps): any { return { type: "Chip", props }; }
