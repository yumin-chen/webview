import { ControlProps } from "../components";
export interface LinkProps extends ControlProps { label: string; url: string; }
export function Link(props: LinkProps): any { return { type: "Link", props }; }
