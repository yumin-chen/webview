import { ComponentProps } from "../components";
export interface DividerProps extends ComponentProps { orientation?: "horizontal" | "vertical"; }
export function Divider(props: DividerProps): any { return { type: "Divider", props }; }
