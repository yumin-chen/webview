import { ComponentProps } from "../components";
export interface ImageProps extends ComponentProps { src: string; }
export function Image(props: ImageProps): any { return { type: "Image", props }; }
