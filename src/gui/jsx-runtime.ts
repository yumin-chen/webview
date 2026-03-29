export function jsx(type: any, props: any, key: any): any {
    if (typeof type === "function") {
        return type(props);
    }
    return { type, props, key };
}

export function jsxs(type: any, props: any, key: any): any {
    return jsx(type, props, key);
}

export const Fragment = (props: any) => props.children;
