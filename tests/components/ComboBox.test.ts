import { expect, test } from "bun:test";
import { ComboBox } from "../../src/gui/components/ComboBox";
test("ComboBox representation", () => {
  const comp = ComboBox({} as any);
  expect(comp.type).toBe("ComboBox");
});
