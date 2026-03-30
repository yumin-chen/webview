import { ControlProps } from "../components";
export interface TextAreaProps extends ControlProps { value?: string; multiline?: boolean; }
export function TextArea(props: TextAreaProps): any { return { type: "TextArea", props }; }
