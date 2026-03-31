import { expect, test } from "bun:test";
import { Label } from "../../src/gui/components/Label";
test("Label representation", () => {
  const comp = Label({} as any);
  expect(comp.type).toBe("Label");
});
