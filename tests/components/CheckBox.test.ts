import { expect, test } from "bun:test";
import { CheckBox } from "../../src/gui/components/CheckBox";
test("CheckBox representation", () => {
  const comp = CheckBox({} as any);
  expect(comp.type).toBe("CheckBox");
});
