import { ComponentProps } from "../components";
export interface IconProps extends ComponentProps { name: string; }
export function Icon(props: IconProps): any { return { type: "Icon", props }; }
