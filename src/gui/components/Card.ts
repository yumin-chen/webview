import { ComponentProps, ReactNode } from "../components";
export interface CardProps extends ComponentProps { children: ReactNode; }
export function Card(props: CardProps): any { return { type: "Card", props }; }
