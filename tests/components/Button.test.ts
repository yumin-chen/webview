import { expect, test, describe } from "bun:test";
import { Button } from "../../src/gui/components/Button";

describe("Button Component", () => {
  test("Button representation and properties", () => {
    const comp = Button({ label: "Click Me", color: "red" });
    expect(comp.type).toBe("Button");
    expect(comp.props.label).toBe("Click Me");
    expect(comp.props.color).toBe("red");
  });

  test("Button with default properties", () => {
    const comp = Button({});
    expect(comp.type).toBe("Button");
    expect(comp.props).toEqual({});
  });

  test("Button event simulation (bridge mock)", async () => {
    let clicked = false;
    const comp = Button({
        label: "Action",
        onClick: () => { clicked = true; }
    });

    // Simulate event from bridge
    if (comp.props.onClick) comp.props.onClick();
    expect(clicked).toBe(true);
  });
});
