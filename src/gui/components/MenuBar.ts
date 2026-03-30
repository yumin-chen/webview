import { ComponentProps, ReactNode } from "../components";
export interface MenuBarProps extends ComponentProps { children: ReactNode[]; }
export function MenuBar(props: MenuBarProps): any { return { type: "MenuBar", props }; }
