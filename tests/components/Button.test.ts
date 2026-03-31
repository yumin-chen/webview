import { expect, test } from "bun:test";
import { Button } from "../../src/gui/components/Button";
test("Button representation", () => {
  const comp = Button({ label: "Click" });
  expect(comp.type).toBe("Button");
  expect(comp.props.label).toBe("Click");
});
