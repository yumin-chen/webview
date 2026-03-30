import { ControlProps } from "../components";
export interface SliderProps extends ControlProps { value: number; min?: number; max?: number; }
export function Slider(props: SliderProps): any { return { type: "Slider", props }; }
