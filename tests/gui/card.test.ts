import { expect, test, describe, spyOn } from "bun:test";
import { Card, createComponent, updateComponent, destroyComponent } from "../../src/gui";

// Mocking window.Alloy for tests
if (typeof window === "undefined") {
    (global as any).window = {};
}
(window as any).Alloy = (window as any).Alloy || {};
(window as any).Alloy.gui = {
    create: (type: string, props: any) => 1,
    update: (id: number, props: any) => {},
    destroy: (id: number) => {},
    addChild: (parent: number, child: number) => {}
};

describe("Alloy:gui > Card", () => {
  test("creation with props", () => {
    const props = { children: [] };
    const element = Card(props);
    expect(element.type).toBe("Card");
    const id = createComponent(element.type, element.props);
    expect(id).toBe(1);
  });

  test("lifecycle: update and destroy", () => {
    const updateSpy = spyOn(window.Alloy.gui, "update");
    const destroySpy = spyOn(window.Alloy.gui, "destroy");

    updateComponent(1, { someProp: "new value" });
    expect(updateSpy).toHaveBeenCalled();

    destroyComponent(1);
    expect(destroySpy).toHaveBeenCalled();
  });
});
