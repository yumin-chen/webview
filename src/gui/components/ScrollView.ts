import { ComponentProps, ReactNode } from "../components";
export interface ScrollViewProps extends ComponentProps { children: ReactNode; }
export function ScrollView(props: ScrollViewProps): any { return { type: "ScrollView", props }; }
