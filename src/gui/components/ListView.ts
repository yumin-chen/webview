import { ControlProps } from "../components";
export interface ListViewProps extends ControlProps { items: any[]; }
export function ListView(props: ListViewProps): any { return { type: "ListView", props }; }
