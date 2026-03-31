import { expect, test } from "bun:test";
import { Toolbar } from "../../src/gui/components/Toolbar";
test("Toolbar representation", () => {
  const comp = Toolbar({} as any);
  expect(comp.type).toBe("Toolbar");
});
