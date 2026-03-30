import { ComponentProps, ReactNode } from "../components";
export interface ContextMenuProps extends ComponentProps { children: ReactNode[]; }
export function ContextMenu(props: ContextMenuProps): any { return { type: "ContextMenu", props }; }
