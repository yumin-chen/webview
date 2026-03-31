import { ComponentProps } from "../components";
export interface LoadingSpinnerProps extends ComponentProps { size?: number; }
export function LoadingSpinner(props: LoadingSpinnerProps): any { return { type: "LoadingSpinner", props }; }
