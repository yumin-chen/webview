import { ComponentProps } from "../components";
export interface ProgressBarProps extends ComponentProps { value: number; indeterminate?: boolean; }
export function ProgressBar(props: ProgressBarProps): any { return { type: "ProgressBar", props }; }
