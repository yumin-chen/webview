import { ComponentProps, ReactNode } from "../components";
export interface ToolbarProps extends ComponentProps { children: ReactNode[]; }
export function Toolbar(props: ToolbarProps): any { return { type: "Toolbar", props }; }
