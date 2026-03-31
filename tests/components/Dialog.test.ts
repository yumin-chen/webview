import { expect, test } from "bun:test";
import { Dialog } from "../../src/gui/components/Dialog";
test("Dialog representation", () => {
  const comp = Dialog({} as any);
  expect(comp.type).toBe("Dialog");
});
