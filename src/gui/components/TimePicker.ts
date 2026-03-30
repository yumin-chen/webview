import { ControlProps } from "../components";
export interface TimePickerProps extends ControlProps { time?: string; }
export function TimePicker(props: TimePickerProps): any { return { type: "TimePicker", props }; }
