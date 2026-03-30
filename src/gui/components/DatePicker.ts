import { ControlProps } from "../components";
export interface DatePickerProps extends ControlProps { date?: Date; }
export function DatePicker(props: DatePickerProps): any { return { type: "DatePicker", props }; }
