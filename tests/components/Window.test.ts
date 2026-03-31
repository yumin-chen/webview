import { expect, test, describe } from "bun:test";
import { Window } from "../../src/gui/components/Window";

describe("Window Component", () => {
  test("Window representation and attributes", () => {
    const comp = Window({ title: "Test Window", width: 800, height: 600 });
    expect(comp.type).toBe("Window");
    expect(comp.props.title).toBe("Test Window");
    expect(comp.props.width).toBe(800);
    expect(comp.props.height).toBe(600);
  });

  test("Window default size", () => {
    const comp = Window({ title: "App" });
    expect(comp.type).toBe("Window");
    expect(comp.props.width).toBeUndefined();
  });
});
