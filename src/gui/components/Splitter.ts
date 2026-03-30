import { ComponentProps, ReactNode } from "../components";
export interface SplitterProps extends ComponentProps { children: [ReactNode, ReactNode]; }
export function Splitter(props: SplitterProps): any { return { type: "Splitter", props }; }
