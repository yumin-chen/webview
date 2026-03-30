import { ComponentProps, ReactNode } from "../components";
export interface WindowProps extends ComponentProps {
    title?: string;
    width?: number;
    height?: number;
    children: ReactNode;
}
export function Window(props: WindowProps): any { return { type: "Window", props }; }
