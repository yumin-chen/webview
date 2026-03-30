import { ComponentProps, ReactNode } from "../components";
export interface TooltipProps extends ComponentProps { text: string; children: ReactNode; }
export function Tooltip(props: TooltipProps): any { return { type: "Tooltip", props }; }
