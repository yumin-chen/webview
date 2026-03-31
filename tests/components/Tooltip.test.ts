import { expect, test } from "bun:test";
import { Tooltip } from "../../src/gui/components/Tooltip";
test("Tooltip representation", () => {
  const comp = Tooltip({} as any);
  expect(comp.type).toBe("Tooltip");
});
