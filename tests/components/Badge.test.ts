import { expect, test } from "bun:test";
import { Badge } from "../../src/gui/components/Badge";
test("Badge representation", () => {
  const comp = Badge({} as any);
  expect(comp.type).toBe("Badge");
});
