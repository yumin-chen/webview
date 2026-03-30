import { ControlProps } from "../components";
export interface RatingProps extends ControlProps { value: number; max?: number; }
export function Rating(props: RatingProps): any { return { type: "Rating", props }; }
