import { ComponentProps, ReactNode } from "../components";
export interface AccordionProps extends ComponentProps { children: ReactNode[]; }
export function Accordion(props: AccordionProps): any { return { type: "Accordion", props }; }
