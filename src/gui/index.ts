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
