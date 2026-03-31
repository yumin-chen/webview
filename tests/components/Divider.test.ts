import { expect, test } from "bun:test";
import { Divider } from "../../src/gui/components/Divider";
test("Divider representation", () => {
  const comp = Divider({} as any);
  expect(comp.type).toBe("Divider");
});
