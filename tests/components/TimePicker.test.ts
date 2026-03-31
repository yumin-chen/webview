import { expect, test } from "bun:test";
import { TimePicker } from "../../src/gui/components/TimePicker";
test("TimePicker representation", () => {
  const comp = TimePicker({} as any);
  expect(comp.type).toBe("TimePicker");
});
