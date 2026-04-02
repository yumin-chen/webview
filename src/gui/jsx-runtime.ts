/*
 * AlloyScript
 * This software is dedicated to the public domain under the CC0 license.
 * To the extent possible under law, the author(s) have dedicated all copyright
 * and related and neighboring rights to this software to the public domain worldwide.
 * This software is distributed without any warranty.
 *
 * DID-based Identity & E2EE IPC Implementation
 */
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
