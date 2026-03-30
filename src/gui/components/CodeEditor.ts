import { ControlProps } from "../components";
export interface CodeEditorProps extends ControlProps { value: string; language?: string; }
export function CodeEditor(props: CodeEditorProps): any { return { type: "CodeEditor", props }; }
