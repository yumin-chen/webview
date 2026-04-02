/*
 * AlloyScript
 * This software is dedicated to the public domain under the CC0 license.
 * To the extent possible under law, the author(s) have dedicated all copyright
 * and related and neighboring rights to this software to the public domain worldwide.
 * This software is distributed without any warranty.
 *
 * DID-based Identity & E2EE IPC Implementation
 */
export interface MouseEvent {
    x: number;
    y: number;
    button: "left" | "right" | "middle";
    clickCount: number;
    modifiers: {
        shift: boolean;
        ctrl: boolean;
        alt: boolean;
        meta: boolean;
    };
}

export interface KeyEvent {
    key: string;
    code: string;
    modifiers: {
        shift: boolean;
        ctrl: boolean;
        alt: boolean;
        meta: boolean;
    };
    repeat: boolean;
}

export interface ResizeEvent {
    width: number;
    height: number;
    previousWidth: number;
    previousHeight: number;
}

export interface MoveEvent {
    x: number;
    y: number;
    previousX: number;
    previousY: number;
}

export interface DropEvent {
    files: string[];
    x: number;
    y: number;
}

export interface WindowState {
    focused: boolean;
    minimized: boolean;
    maximized: boolean;
    fullscreen: boolean;
}
