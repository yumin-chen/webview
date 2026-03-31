import { expect, test } from "bun:test";
import { Switch } from "../../src/gui/components/Switch";
test("Switch representation", () => {
  const comp = Switch({} as any);
  expect(comp.type).toBe("Switch");
});
