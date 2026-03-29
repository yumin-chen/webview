import { expect, test, describe, beforeEach } from "bun:test";
import { createComponent, updateComponent, destroyComponent, Window, Button, TextField } from "../src/gui";

describe("Alloy:gui End-to-End Bridge Tests", () => {
  let createdComponents: any[] = [];

  beforeEach(() => {
    createdComponents = [];
    (global as any).window = (global as any).window || {};
    (global as any).window.Alloy = {
        gui: {
            create: (type: string, props: any) => {
                const handle = `0x${Math.floor(Math.random()*1000000).toString(16)}`;
                createdComponents.push({ handle, type, props });
                return handle;
            },
            update: (handle: string, props: any) => {
                const comp = createdComponents.find(c => c.handle === handle);
                if (comp) comp.props = { ...comp.props, ...props };
            },
            destroy: (handle: string) => {
                createdComponents = createdComponents.filter(c => c.handle !== handle);
            }
        }
    };
  });

  test("Full application lifecycle simulation", () => {
    // 1. Create main window
    const winHandle = createComponent("Window", { title: "Main App", width: 800, height: 600 });
    expect(winHandle).toMatch(/^0x/);
    expect(createdComponents).toHaveLength(1);
    expect(createdComponents[0].type).toBe("Window");

    // 2. Create sub-components
    const btnHandle = createComponent("Button", { label: "Submit" });
    const tfHandle = createComponent("TextField", { placeholder: "Name" });
    expect(createdComponents).toHaveLength(3);

    // 3. Update a component
    updateComponent(btnHandle, { label: "Loading..." });
    const btn = createdComponents.find(c => c.handle === btnHandle);
    expect(btn.props.label).toBe("Loading...");

    // 4. Destroy a component
    destroyComponent(tfHandle);
    expect(createdComponents).toHaveLength(2);
    expect(createdComponents.find(c => c.handle === tfHandle)).toBeUndefined();

    // 5. Cleanup app
    destroyComponent(winHandle);
    destroyComponent(btnHandle);
    expect(createdComponents).toHaveLength(0);
  });

  test("Event routing simulation (JS side)", () => {
      let clickCount = 0;
      const btnProps = {
          label: "Counter",
          onClick: () => { clickCount++; }
      };
      const btnHandle = createComponent("Button", btnProps);

      // Simulate native event coming through the bridge
      const comp = createdComponents.find(c => c.handle === btnHandle);
      if (comp && comp.props.onClick) {
          comp.props.onClick();
      }

      expect(clickCount).toBe(1);
  });
});
