import { ComponentProps, ReactNode } from "../components";
export interface DialogProps extends ComponentProps { title: string; children: ReactNode; }
export function Dialog(props: DialogProps): any { return { type: "Dialog", props }; }
