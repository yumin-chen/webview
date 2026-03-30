import { ComponentProps } from "../components";
export interface LabelProps extends ComponentProps { text: string; }
export function Label(props: LabelProps): any { return { type: "Label", props }; }
