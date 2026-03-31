import { expect, test } from "bun:test";
import { Splitter } from "../../src/gui/components/Splitter";
test("Splitter representation", () => {
  const comp = Splitter({} as any);
  expect(comp.type).toBe("Splitter");
});
