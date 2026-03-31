import { expect, test } from "bun:test";
import { DatePicker } from "../../src/gui/components/DatePicker";
test("DatePicker representation", () => {
  const comp = DatePicker({} as any);
  expect(comp.type).toBe("DatePicker");
});
