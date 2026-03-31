import { expect, test } from "bun:test";
import { Menu } from "../../src/gui/components/Menu";
test("Menu representation", () => {
  const comp = Menu({} as any);
  expect(comp.type).toBe("Menu");
});
