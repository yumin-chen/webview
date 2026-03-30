import { ComponentProps, ReactNode } from "../components";
export interface PopoverProps extends ComponentProps { children: ReactNode; }
export function Popover(props: PopoverProps): any { return { type: "Popover", props }; }
