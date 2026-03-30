import { ComponentProps } from "../components";
export interface WebViewProps extends ComponentProps { src?: string; html?: string; }
export function WebView(props: WebViewProps): any { return { type: "WebView", props }; }
