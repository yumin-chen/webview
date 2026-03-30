import { ComponentProps, ReactNode } from "../components";
export interface MenuProps extends ComponentProps { label: string; children: ReactNode[]; }
export function Menu(props: MenuProps): any { return { type: "Menu", props }; }
