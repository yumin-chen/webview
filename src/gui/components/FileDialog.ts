import { ComponentProps } from "../components";
export interface FileDialogProps extends ComponentProps { mode: "open" | "save"; }
export function FileDialog(props: FileDialogProps): any { return { type: "FileDialog", props }; }
