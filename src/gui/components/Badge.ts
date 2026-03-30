import { ComponentProps } from "../components";
export interface BadgeProps extends ComponentProps { text: string; }
export function Badge(props: BadgeProps): any { return { type: "Badge", props }; }
