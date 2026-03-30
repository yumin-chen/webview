/**
 * @alloyscript/runtime GUI components
 */

const wrap = (type: string) => async (props: any) => {
    const handle = await (window as any).Alloy.gui.create(type, props);
    return {
        handle,
        type,
        props,
        addChild(child: any) {
            child.props.parent = this.handle;
            return (window as any).Alloy.gui.create(child.type, child.props);
        }
    };
};

export const Window = wrap("Window");
export const Button = wrap("Button");
export const TextField = wrap("TextField");
export const TextArea = wrap("TextArea");
export const Label = wrap("Label");
export const CheckBox = wrap("CheckBox");
export const RadioButton = wrap("RadioButton");
export const ComboBox = wrap("ComboBox");
export const Slider = wrap("Slider");
export const ProgressBar = wrap("ProgressBar");
export const TabView = wrap("TabView");
export const ListView = wrap("ListView");
export const TreeView = wrap("TreeView");
export const WebView = wrap("WebView");
export const VStack = wrap("VStack");
export const HStack = wrap("HStack");
export const ScrollView = wrap("ScrollView");

export function useSignal<T>(initial: T): any {
    return (window as any).Alloy.gui.createSignal(initial);
}

export const Color = {
    rgb: (r: number, g: number, b: number) => `rgb(${r},${g},${b})`,
    hex: (h: string) => h,
    white: () => "#ffffff",
    black: () => "#000000",
    blue: (v: number) => `blue-${v}`,
};
