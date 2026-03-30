import { ControlProps } from "../components";
export interface TabViewProps extends ControlProps { tabs: any[]; }
export function TabView(props: TabViewProps): any { return { type: "TabView", props }; }
