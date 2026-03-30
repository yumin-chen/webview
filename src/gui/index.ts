export const Window = (props: any) => ({ type: "Window", props });
export const Button = (props: any) => ({ type: "Button", props });
export const useSignal = (initial: any) => (window as any).Alloy.gui.createSignal(initial);
