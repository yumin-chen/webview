import { ComponentProps, ReactNode } from "../components";
export interface GroupBoxProps extends ComponentProps { label?: string; children: ReactNode; }
export function GroupBox(props: GroupBoxProps): any { return { type: "GroupBox", props }; }
