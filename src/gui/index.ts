/*
 * AlloyScript
 * This software is dedicated to the public domain under the CC0 license.
 * To the extent possible under law, the author(s) have dedicated all copyright
 * and related and neighboring rights to this software to the public domain worldwide.
 * This software is distributed without any warranty.
 *
 * DID-based Identity & E2EE IPC Implementation
 */
import { Window, Button, TextField, VStack, HStack } from "./components";
import { Color } from "./types";

declare global {
  interface Window {
    Alloy: {
      gui: {
        create: (type: string, props: any) => number; // returns component_id
        update: (id: number, props: any) => void;
        destroy: (id: number) => void;
      };
    };
  }
}

export {
    Window,
    Button,
    TextField,
    VStack,
    HStack,
    Color
};

export const createComponent = (type: string, props: any) => {
    return window.Alloy.gui.create(type, props);
};

export const updateComponent = (id: number, props: any) => {
    window.Alloy.gui.update(id, props);
};

export const destroyComponent = (id: number) => {
    window.Alloy.gui.destroy(id);
};
