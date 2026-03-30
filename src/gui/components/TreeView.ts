import { ControlProps } from "../components";
export interface TreeViewProps extends ControlProps { root: any; }
export function TreeView(props: TreeViewProps): any { return { type: "TreeView", props }; }
