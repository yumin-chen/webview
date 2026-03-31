import { expect, test } from "bun:test";
import { ColorPicker } from "../../src/gui/components/ColorPicker";
test("ColorPicker representation", () => {
  const comp = ColorPicker({} as any);
  expect(comp.type).toBe("ColorPicker");
});
