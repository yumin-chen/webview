import { expect, test } from "bun:test";
import { Separator } from "../../src/gui/components/Separator";
test("Separator representation", () => {
  const comp = Separator({});
  expect(comp.type).toBe("Separator");
});
