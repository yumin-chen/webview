import { expect, test } from "bun:test";
import { Popover } from "../../src/gui/components/Popover";
test("Popover representation", () => {
  const comp = Popover({} as any);
  expect(comp.type).toBe("Popover");
});
