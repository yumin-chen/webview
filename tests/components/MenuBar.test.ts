import { expect, test } from "bun:test";
import { MenuBar } from "../../src/gui/components/MenuBar";
test("MenuBar representation", () => {
  const comp = MenuBar({} as any);
  expect(comp.type).toBe("MenuBar");
});
