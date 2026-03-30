import { ControlProps } from "../components";
export interface RichTextEditorProps extends ControlProps { value: string; }
export function RichTextEditor(props: RichTextEditorProps): any { return { type: "RichTextEditor", props }; }
