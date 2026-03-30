import { ComponentProps, Padding, ReactNode } from "../components";
export interface StackProps extends ComponentProps { spacing?: number; padding?: Padding; children: ReactNode[]; }
export function VStack(props: StackProps): any { return { type: "VStack", props }; }
