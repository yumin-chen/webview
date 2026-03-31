import { expect, test } from "bun:test";
import { Accordion } from "../../src/gui/components/Accordion";
test("Accordion representation", () => {
  const comp = Accordion({} as any);
  expect(comp.type).toBe("Accordion");
});
