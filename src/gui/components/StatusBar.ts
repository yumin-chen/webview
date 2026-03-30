import { ComponentProps } from "../components";
export interface StatusBarProps extends ComponentProps { text?: string; }
export function StatusBar(props: StatusBarProps): any { return { type: "StatusBar", props }; }
