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
