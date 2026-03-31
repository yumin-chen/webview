import { expect, test } from "bun:test";
import { RadioButton } from "../../src/gui/components/RadioButton";
test("RadioButton representation", () => {
  const comp = RadioButton({} as any);
  expect(comp.type).toBe("RadioButton");
});
