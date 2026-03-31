import { ComponentProps } from "../components";
export interface SeparatorProps extends ComponentProps { orientation?: "horizontal" | "vertical"; }
export function Separator(props: SeparatorProps): any { return { type: "Separator", props }; }
